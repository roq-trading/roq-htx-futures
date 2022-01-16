/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/rest.h"

#include <algorithm>
#include <utility>

#include "roq/utils/mask.h"
#include "roq/utils/safe_cast.h"
#include "roq/utils/update.h"

#include "roq/core/back_emplacer.h"
#include "roq/core/charconv.h"

#include "roq/core/json/parser.h"

#include "roq/core/metrics/factory.h"

#include "roq/huobi_futures/flags.h"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

namespace {
static const auto NAME = "rest"sv;

static const auto SUPPORTS = utils::Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
};

static const auto ALLOW_PIPELINING = true;

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

Rest::Rest(Handler &handler, core::io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      connection_(
          *this,
          context,
          Flags::decode_buffer_size(),
          Flags::encode_buffer_size(),
          core::URI(Flags::rest_uri()),
          ROQ_PACKAGE_NAME,
          core::http::Connection::KEEP_ALIVE,
          ALLOW_PIPELINING,
          Flags::rest_request_timeout(),
          Flags::rest_ping_freq(),
          Flags::rest_ping_path()),
      decode_buffer_(Flags::decode_buffer_size()),
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
      shared_(shared),
      download_(Flags::rest_request_timeout(), [this](auto state) { return download(state); }) {
}

void Rest::operator()(const Event<Start> &) {
  connection_.start();
}

void Rest::operator()(const Event<Stop> &) {
  connection_.stop();
}

void Rest::operator()(const Event<Timer> &event) {
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
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::REST,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void Rest::operator()(const core::web::Client::Connected &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void Rest::operator()(const core::web::Client::Disconnected &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void Rest::operator()(const core::web::Client::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

uint32_t Rest::download(RestState state) {
  switch (state) {
    case RestState::UNDEFINED:
      assert(false);
      break;
    case RestState::CONTRACT_INFO:
      get_contract_info();
      return 1;
    case RestState::DONE:
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
    connection_(
        "contract_info"sv,
        request,
        [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          auto trace_info = server::create_trace_info();
          server::Trace event(trace_info, response);
          get_contract_info_ack(event, sequence);
        });
  });
}

void Rest::get_contract_info_ack(
    const server::Trace<core::web::Response> &event, uint32_t sequence) {
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
      auto contract_info = core::json::Parser::create<json::ContractInfo>(body, buffer);
      server::Trace event(trace_info, contract_info);
      (*this)(event);
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void Rest::operator()(const server::Trace<json::ContractInfo> &event) {
  auto &[trace_info, contract_info] = event;
  log::info<4>("contract_info={}"sv, contract_info);
  std::vector<std::string> symbols;
  symbols.reserve(std::size(contract_info.data));
  size_t counter = 0;
  for (size_t i = 0; i < std::size(contract_info.data); ++i) {
    auto &item = contract_info.data[i];
    log::info<2>("item={}"sv, item);
    auto symbol = item.contract_code;
    if (shared_.discard_symbol(symbol))
      continue;
    if (item.contract_status != 1) {
      log::warn<1>(
          R"(Dropping pair="{}" due to contract_status={})"sv, item.pair, item.contract_status);
      continue;
    }
    if (all_symbols_.emplace(symbol).second)  // only include new
      symbols.emplace_back(symbol);
    ++counter;
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
        .min_trade_vol = NaN,
        .max_trade_vol = NaN,
        .trade_vol_step_size = NaN,
        .option_type = {},
        .strike_currency = {},
        .strike_price = NaN,
        .underlying = {},
        .time_zone = {},
        .issue_date = {},
        .settlement_date = utils::safe_cast(item.settlement_time),
        .expiry_datetime = {},
        .expiry_datetime_utc = {},
    };
    server::create_trace_and_dispatch(handler_, trace_info, reference_data, true);
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
