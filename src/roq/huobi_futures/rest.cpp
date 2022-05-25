/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/rest.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/back_emplacer.hpp"
#include "roq/core/charconv.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/huobi_futures/flags.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

namespace {
auto const NAME = "rest"sv;

const Mask SUPPORTS{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(std::string_view const &group, std::string_view const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::rest_uri();
  core::web::Client::Config config{
      .decode_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
      .validate_certificate = server::Flags::tls_validate_certificate(),
      .uris = {&uri, 1},
      .proxy = Flags::rest_proxy(),
      .user_agent = ROQ_PACKAGE_NAME,
      .connection = core::http::Connection::KEEP_ALIVE,
      .allow_pipelining = true,
      .request_timeout = Flags::rest_request_timeout(),
      .ping_frequency = Flags::rest_ping_freq(),
      .ping_path = Flags::rest_ping_path(),
  };
  return core::web::Client{handler, context, config};
}
}  // namespace

Rest::Rest(Handler &handler, core::io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      connection_(create_connection(*this, context)), decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .contract_info = create_metrics(name_, "contract_info"sv),
          .contract_info_ack = create_metrics(name_, "contract_info_ack"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
      },
      shared_(shared), download_(Flags::rest_request_timeout(), [this](auto state) { return download(state); }) {
}

void Rest::operator()(Event<Start> const &) {
  connection_.start();
}

void Rest::operator()(Event<Stop> const &) {
  connection_.stop();
}

void Rest::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  connection_.refresh(now);
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
    auto trace_info = server::create_trace_info();
    const StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::HTTP,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void Rest::operator()(core::web::Client::Connected const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void Rest::operator()(core::web::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void Rest::operator()(core::web::Client::Latency const &latency) {
  auto trace_info = server::create_trace_info();
  const ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
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
    case DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

// contract info

void Rest::get_contract_info() {
  profile_.contract_info([&]() {
    auto method = core::http::Method::GET;
    auto path = shared_.api.get_contract_info;
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto sequence = download_.sequence();
    connection_("contract_info"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
      auto trace_info = server::create_trace_info();
      Trace event(trace_info, response);
      get_contract_info_ack(event, sequence);
    });
  });
}

void Rest::get_contract_info_ack(Trace<core::web::Response const> const &event, uint32_t sequence) {
  profile_.contract_info_ack([&]() {
    auto &[trace_info, response] = event;
    auto state = RestState::CONTRACT_INFO;
    try {
      auto [status, category, body] = response.result();
      log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
      if (download_.skip(sequence, state)) {
        log::info("Download state={} has already been processed"sv, state);
        return;
      }
      response.expect(core::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      const auto contract_info = core::json::Parser::create<json::ContractInfo>(body, buffer);
      Trace event(trace_info, contract_info);
      (*this)(event);
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void Rest::operator()(Trace<json::ContractInfo const> const &event) {
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
    const ReferenceData reference_data{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .description = item.contract_code,
        .security_type = {},
        .base_currency = {},
        .quote_currency = {},
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = item.price_tick,
        .multiplier = item.contract_size,
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
    SymbolsUpdate symbols_update{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
  if (counter > 0) [[unlikely]]
    log::info("Symbols {} / {}"sv, counter, std::size(contract_info.data));
}

}  // namespace huobi_futures
}  // namespace roq
