/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/gateway/rest.hpp"

#include "roq/mask.hpp"

#include "roq/utils/safe_cast.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/core/json/parser.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "rest"sv;

auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 1;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context, auto &shared) {
  auto uri = settings.rest.uri;
  auto config = web::rest::Client::Config{
      // connection
      .interface = {},
      .proxy = settings.rest.proxy,
      .uris = {&uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = {},
      .disconnect_on_idle_timeout = {},
      .connection = web::http::Connection::KEEP_ALIVE,
      // request
      .allow_pipelining = true,
      .request_timeout = settings.rest.request_timeout,
      // response
      .suspend_on_retry_after = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .ping_frequency = settings.rest.ping_freq,
      .ping_path = settings.rest.ping_path,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::rest::Client::create(handler, context, config, shared.throttle);
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

Rest::Rest(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, connection_{create_connection(*this, shared.settings, context, shared)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .contract_info = create_metrics(shared.settings, name_, "contract_info"sv),
          .contract_info_ack = create_metrics(shared.settings, name_, "contract_info_ack"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      shared_{shared}, download_{shared.settings.rest.request_timeout, [this](auto &event) { return download(event); }} {
}

// server::Stream

void Rest::operator()(Trace<Start> const &) {
  (*connection_).start();
}

void Rest::operator()(Trace<Stop> const &) {
  (*connection_).stop();
}

void Rest::operator()(Trace<Timer> const &event) {
  auto &[trace_info, timer] = event;
  if ((*connection_).refresh(timer.now)) {
    if (ready() && next_refresh_.count() && next_refresh_ < timer.now && !download_.downloading()) {
      next_refresh_ = {};
      download_.reset();
      download_.begin(trace_info);
    }
  }
}

void Rest::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.contract_info, metrics::Type::PROFILE)
      .write(profile_.contract_info_ack, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

void Rest::operator()(Trace<ConnectionStatus> const &event, std::string_view const &reason) {
  auto &[trace_info, connection_status] = event;
  connection_status_ = connection_status;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = {},
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::HTTP,
      .encoding = {Encoding::JSON},
      .priority = Priority::PRIMARY,
      .connection_status = connection_status_,
      .reason = reason,
      .interface = (*connection_).get_interface(),
      .authority = (*connection_).get_current_authority(),
      .path = (*connection_).get_current_path(),
      .proxy = (*connection_).get_proxy(),
  };
  log::info("stream_status={}"sv, stream_status);
  create_trace_and_dispatch(shared_.dispatcher, trace_info, stream_status);
}

// web::rest::Client::Handler

void Rest::operator()(Trace<web::rest::Connected> const &event) {
  auto &[trace_info, connected] = event;
  if (download_.downloading()) {
    download_.bump(trace_info);
  } else {
    download_.begin(trace_info);
  }
}

void Rest::operator()(Trace<web::rest::Disconnected> const &event) {
  auto &[trace_info, disconnected] = event;
  ++counter_.disconnect;
  create_trace_and_dispatch_2(trace_info, ConnectionStatus::DISCONNECTED);
  if (!download_.downloading()) {
    download_.reset();
  }
  next_refresh_ = {};
}

void Rest::operator()(Trace<web::rest::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

// core::Download

int32_t Rest::download(Trace<State> const &event) {
  auto &[trace_info, state] = event;
  switch (state) {
    using enum State;
    case UNDEFINED:
      assert(false);
      break;
    case CONTRACT_INFO:
      create_trace_and_dispatch_2(trace_info, ConnectionStatus::DOWNLOADING, "contract-info"sv);
      get_contract_info();
      return 1;
    case DONE: {
      create_trace_and_dispatch_2(trace_info, ConnectionStatus::READY);
      auto period = shared_.settings.rest.download_refresh;
      if (period.count()) {
        auto now = clock::get_system();
        next_refresh_ = now + period;
      }
      return {};
    }
  }
  assert(false);
  return {};
}

// contract-info

void Rest::get_contract_info() {
  profile_.contract_info([&]() {
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = shared_.api.market_data.get_contract_info,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()](auto &event, [[maybe_unused]] auto &request_id) { get_contract_info_ack(event, sequence); };
    (*connection_)(request, callback, "contract_info"sv);
  });
}

void Rest::get_contract_info_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::CONTRACT_INFO;
  profile_.contract_info_ack([&]() {
    auto &[trace_info, response] = event;
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        protocol::json::ContractInfoAck contract_info_ack{body, decode_buffer_};
        // XXX debug -- saw something 20220603 -- maybe like this
        if (std::empty(contract_info_ack.data)) {
          log::warn(R"(DEBUG: body="{}")"sv, body);
        }
        create_trace_and_dispatch_2(trace_info, contract_info_ack);
        download_.check(trace_info, STATE);
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void Rest::operator()(Trace<protocol::json::ContractInfoAck> const &event) {
  auto &[trace_info, contract_info_ack] = event;
  log::info<4>("contract_info_ack={}"sv, contract_info_ack);
  std::vector<Symbol> symbols;
  symbols.reserve(std::size(contract_info_ack.data));
  size_t counter = 0;
  for (size_t i = 0; i < std::size(contract_info_ack.data); ++i) {
    auto &item = contract_info_ack.data[i];
    log::info<2>("item={}"sv, item);
    if (item.contract_status != 1) {
      log::warn<1>(R"(Dropping pair="{}" due to contract_status={})"sv, item.pair, item.contract_status);
      continue;
    }
    auto symbol = item.contract_code;
    auto discard = shared_.dispatcher.discard_symbol(symbol);
    auto reference_data = ReferenceData{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol,
        .description = item.contract_code,
        .security_type = {},
        .external_security_id = {},
        .market_segment = {},
        .cfi_code = {},
        .base_currency = {},
        .quote_currency = {},
        .settlement_currency = {},
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = item.price_tick,
        .tick_size_steps = {},
        .multiplier = item.contract_size,
        .min_notional = NaN,
        .min_trade_vol = 1.0,  // lots
        .max_trade_vol = NaN,
        .trade_vol_step_size = 1.0,  // lots
        .option_type = {},
        .strike_currency = {},
        .strike_price = NaN,
        .underlying = {},
        .time_zone = {},
        .issue_date = {},
        .settlement_date = utils::safe_cast(item.settlement_time),
        .expiry_datetime = {},
        .expiry_datetime_utc = {},
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
        .discard = discard,
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, reference_data, true);
    if (discard) {
      log::info<1>(R"(Drop symbol="{}")"sv, item.symbol);
      continue;
    }
    if (shared_.all_symbols.emplace(symbol).second) {  // only include new
      symbols.emplace_back(symbol);
    }
    ++counter;
  }
  if (!std::empty(symbols)) {
    auto symbols_update = SymbolsUpdate{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
  if (counter > 0) [[unlikely]] {
    log::info("Symbols {} / {}"sv, counter, std::size(contract_info_ack.data));
  }
}

// helpers

void Rest::process_response(Trace<web::rest::Response> const &event, auto error_handler, auto success_handler) {
  auto &[trace, response] = event;
  try {
    auto [status, category, body] = response.result();
    log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
    switch (category) {
      using enum web::http::Category;
      case SUCCESS:  // 2xx
        success_handler(body);
        break;
      case CLIENT_ERROR:    // 4xx
      case SERVER_ERROR: {  // 5xx
        auto text = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, text);
        break;
      }
      default:
        response.expect(web::http::Status::OK);  // throws
    }
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::GATEWAY, e.request_status(), e.error(), e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, e.what());
  }
}

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
