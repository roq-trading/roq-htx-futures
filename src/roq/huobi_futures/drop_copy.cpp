/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi_futures/drop_copy.h"

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/metrics/factory.h"

#include "roq/huobi_futures/flags.h"

#include "roq/huobi_futures/json/utils.h"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

namespace {
static const auto NAME = "ex"sv;
static const auto SUPPORTS = utils::Mask{
    SupportType::FUNDS,
};

static auto create_query(const std::string_view &listen_key) {
  assert(!listen_key.empty());
  return fmt::format("?streams={}"sv, listen_key);
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

DropCopy::DropCopy(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared,
    const std::string_view &listen_key)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      connection_(
          *this,
          context,
          core::URI(Flags::ws_order_uri()),
          create_query(listen_key),
          Flags::ws_ping_freq(),
          Flags::decode_buffer_size(),
          Flags::encode_buffer_size(),
          []() { return std::string(); }),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
          .outbound_account_info = create_metrics(name_, "outbound_account_info"sv),
          .outbound_account_position = create_metrics(name_, "outbound_account_position"sv),
          .balance_update = create_metrics(name_, "balance_update"sv),
          .execution_report = create_metrics(name_, "execution_report"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
      },
      security_(security), shared_(shared),
      download_({}, [this](auto state) { return download(state); }) {
}

bool DropCopy::ready() const {
  return connection_.ready();
}

void DropCopy::operator()(const Event<Start> &) {
  connection_.start();
}

void DropCopy::operator()(const Event<Stop> &) {
  connection_.stop();
}

void DropCopy::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void DropCopy::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.outbound_account_info, metrics::PROFILE)
      .write(profile_.outbound_account_position, metrics::PROFILE)
      .write(profile_.balance_update, metrics::PROFILE)
      .write(profile_.execution_report, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void DropCopy::operator()(const core::web::Socket::Connected &) {
}

void DropCopy::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void DropCopy::operator()(const core::web::Socket::Ready &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void DropCopy::operator()(const core::web::Socket::Close &) {
}

void DropCopy::operator()(const core::web::Socket::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void DropCopy::operator()(const core::web::Socket::Binary &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

uint32_t DropCopy::download(DropCopyState state) {
  switch (state) {
    case DropCopyState::UNDEFINED:
      assert(false);
      break;
    case DropCopyState::DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      // subscribe(symbols_);
      return {};
  }
  assert(false);
  return {};
}

void DropCopy::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      auto trace_info = server::create_trace_info();
      core::json::Buffer buffer(decode_buffer_);
      json::UserStreamParser::dispatch(*this, message, buffer, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void DropCopy::operator()(
    const json::OutboundAccountInfo &outbound_account_info, const server::TraceInfo &trace_info) {
  profile_.outbound_account_info([&]() {
    log::info<3>("outbound_account_info={}"sv, outbound_account_info);
    for (auto &item : outbound_account_info.balances) {
      FundsUpdate funds_update{
          .stream_id = stream_id_,
          .account = security_.get_account(),
          .currency = item.asset,
          .balance = item.free_amount,
          .hold = item.locked_amount,
          .external_account = {},
      };
      create_trace_and_dispatch(handler_, trace_info, funds_update, true);
    }
  });
}

void DropCopy::operator()(
    const json::OutboundAccountPosition &outbound_account_position,
    const server::TraceInfo &trace_info) {
  profile_.outbound_account_position([&]() {
    log::info<3>("outbound_account_position={}"sv, outbound_account_position);
    for (auto &item : outbound_account_position.balances) {
      FundsUpdate funds_update{
          .stream_id = stream_id_,
          .account = security_.get_account(),
          .currency = item.asset,
          .balance = item.free_amount,
          .hold = item.locked_amount,
          .external_account = {},
      };
      create_trace_and_dispatch(handler_, trace_info, funds_update, true);
    }
  });
}

void DropCopy::operator()(const json::BalanceUpdate &balance_update, const server::TraceInfo &) {
  profile_.balance_update([&]() {
    log::info<3>("balance_update={}"sv, balance_update);
    // note! contains delta (changes) -- we're not going to use here
  });
}

void DropCopy::operator()(
    const json::ExecutionReport &execution_report, const server::TraceInfo &trace_info) {
  profile_.execution_report([&]() {
    log::info<3>("execution_report={}"sv, execution_report);
    auto side = json::map(execution_report.side);
    auto status = json::map(execution_report.current_order_status);
    oms::OrderUpdate order_update{
        .account = security_.get_account(),
        .exchange = Flags::exchange(),
        .symbol = execution_report.symbol,
        .side = side,
        .position_effect = {},
        .max_show_quantity = NaN,
        .order_type = {},
        .time_in_force = {},
        .execution_instruction = {},
        .order_template = {},
        .create_time_utc = {},
        .update_time_utc = execution_report.transaction_time,
        .external_account = {},
        .external_order_id = execution_report.client_order_id,
        .status = status,
        .quantity = NaN,
        .price = execution_report.price,
        .stop_price = NaN,
        .remaining_quantity = NaN,
        .traded_quantity = execution_report.cumulative_filled_quantity,
        .average_traded_price = NaN,
        .last_traded_quantity = NaN,
        .last_traded_price = NaN,
        .last_liquidity = {},
    };
    if (shared_.update_order(
            execution_report.client_order_id,
            stream_id_,
            trace_info,
            order_update,
            [&]([[maybe_unused]] auto &order) {
              // XXX IMPLEMENT
            })) {
    } else {
      log::warn("*** EXTERNAL ORDER ***"sv);
      log::warn("execution_report={}"sv, execution_report);
    }
  });
}

}  // namespace huobi_futures
}  // namespace roq
