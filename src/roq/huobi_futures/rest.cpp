/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/huobi_futures/rest.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/charconv.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/rest/client_factory.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

// === CONSTANTS ===

namespace {
auto const NAME = "rest"sv;

auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
};
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.rest.uri;
  auto config = web::rest::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = {},
      .disconnect_on_idle_timeout = {},
      .connection = web::http::Connection::KEEP_ALIVE,
      // proxy
      .proxy = settings.rest.proxy,
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = settings.rest.request_timeout,
      .ping_frequency = settings.rest.ping_freq,
      .ping_path = settings.rest.ping_path,
      // implementation
      .decode_buffer_size = settings.common.decode_buffer_size,
      .encode_buffer_size = settings.common.encode_buffer_size,
      .allow_pipelining = true,
  };
  return web::rest::ClientFactory::create(handler, context, config);
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(auto &settings, auto const &group, auto const &function)
      : core::metrics::Factory(settings.app.name, group, function) {}
};
}  // namespace

// === IMPLEMENTATION ===

Rest::Rest(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)},
      connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_(shared.settings.common.decode_buffer_size),
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
      shared_{shared}, download_{shared.settings.rest.request_timeout, [this](auto state) { return download(state); }} {
}

void Rest::operator()(Event<Start> const &) {
  (*connection_).start();
}

void Rest::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void Rest::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  (*connection_).refresh(now);
  if (ready() && next_refresh_.count() && next_refresh_ < now && !download_.downloading()) {
    next_refresh_ = {};
    download_.reset();
    download_.begin();
  }
}

void Rest::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.contract_info, metrics::PROFILE)
      .write(profile_.contract_info_ack, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

void Rest::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::HTTP,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
        .interface = (*connection_).get_interface(),
        .authority = (*connection_).get_current_authority(),
        .path = (*connection_).get_current_path(),
        .proxy = (*connection_).get_proxy(),
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void Rest::operator()(Trace<web::rest::Client::Connected> const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void Rest::operator()(Trace<web::rest::Client::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
  next_refresh_ = {};
}

void Rest::operator()(Trace<web::rest::Client::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void Rest::operator()(
    Trace<web::rest::Response> const &, [[maybe_unused]] uint64_t request_id, [[maybe_unused]] uint64_t opaque) {
}

uint32_t Rest::download(RestState state) {
  switch (state) {
    using enum RestState;
    case UNDEFINED:
      assert(false);
      break;
    case CONTRACT_INFO:
      get_contract_info();
      return 1;
    case DONE: {
      (*this)(ConnectionStatus::READY);
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

// contract info

void Rest::get_contract_info() {
  profile_.contract_info([&]() {
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = shared_.api.get_contract_info,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_contract_info_ack(event, sequence);
    };
    (*connection_)("contract_info"sv, request, callback);
  });
}

void Rest::get_contract_info_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  constexpr auto const STATE = RestState::CONTRACT_INFO;
  profile_.contract_info_ack([&]() {
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        auto contract_info = json::ContractInfo::create(body, decode_buffer_);
        // XXX debug -- saw something 20220603 -- maybe like this
        if (std::empty(contract_info.data)) {
          log::warn(R"(DEBUG: body="{}")"sv, body);
        }
        Trace event_2{event, contract_info};
        (*this)(event_2);
        download_.check(STATE);
      }
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      download_.retry(STATE);
    };
    process_response(event, handle_success, handle_error);
  });
}

void Rest::operator()(Trace<json::ContractInfo> const &event) {
  auto &[trace_info, contract_info] = event;
  log::info<4>("contract_info={}"sv, contract_info);
  std::vector<Symbol> symbols;
  symbols.reserve(std::size(contract_info.data));
  size_t counter = 0;
  for (size_t i = 0; i < std::size(contract_info.data); ++i) {
    auto &item = contract_info.data[i];
    log::info<2>("item={}"sv, item);
    if (item.contract_status != 1) {
      log::warn<1>(R"(Dropping pair="{}" due to contract_status={})"sv, item.pair, item.contract_status);
      continue;
    }
    auto symbol = item.contract_code;
    auto discard = shared_.discard_symbol(symbol);
    auto reference_data = ReferenceData{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol,
        .description = item.contract_code,
        .security_type = {},
        .base_currency = {},
        .quote_currency = {},
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = item.price_tick,
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
        .discard = discard,
    };
    create_trace_and_dispatch(handler_, trace_info, reference_data, true);
    if (discard)
      continue;
    if (all_symbols_.emplace(symbol).second)  // only include new
      symbols.emplace_back(symbol);
    ++counter;
  }
  if (!std::empty(symbols)) {
    auto symbols_update = SymbolsUpdate{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
  if (counter > 0) [[unlikely]]
    log::info("Symbols {} / {}"sv, counter, std::size(contract_info.data));
}

template <typename SuccessHandler, typename ErrorHandler>
void Rest::process_response(
    web::rest::Response const &response, SuccessHandler success_handler, ErrorHandler error_handler) {
  try {
    auto [status, category, body] = response.result();
    log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
    switch (category) {
      using enum web::http::Category;
      case SUCCESS:  // 2xx
        success_handler(body);
        break;
      case CLIENT_ERROR: {  // 4xx
        auto text = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, text);
        break;
      }
      case SERVER_ERROR: {  // 5xx
        auto text = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, text);
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

}  // namespace huobi_futures
}  // namespace roq
