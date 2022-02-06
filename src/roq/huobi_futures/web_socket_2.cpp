/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/web_socket_2.h"

#include "roq/utils/mask.h"
#include "roq/utils/safe_cast.h"
#include "roq/utils/update.h"

#include "roq/core/metrics/factory.h"

#include "roq/huobi_futures/flags.h"

#include "roq/huobi_futures/json/utils.h"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

namespace {
const auto NAME = "ws2"sv;
const auto SUPPORTS = utils::Mask{
    SupportType::STATISTICS,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

WebSocket2::WebSocket2(
    Handler &handler, core::io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      index_(index), connection_(
                         *this,
                         context,
                         core::URI(Flags::ws_order_uri()),
                         {},  // query
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
          .ping = create_metrics(name_, "ping"sv),
          .close = create_metrics(name_, "close"sv),
          .funding_rate = create_metrics(name_, "funding_rate"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
      },
      shared_(shared), inflate_(core::zlib::Inflate::GZIP_NO_HEADER) {
}

void WebSocket2::operator()(const Event<Start> &) {
  connection_.start();
}

void WebSocket2::operator()(const Event<Stop> &) {
  connection_.stop();
}

void WebSocket2::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void WebSocket2::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.funding_rate, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

void WebSocket2::subscribe(size_t start_from) {
  if (ready())
    subscribe(shared_.symbols.get_slice(index_, start_from));
}

void WebSocket2::operator()(const core::web::ClientSocket::Connected &) {
}

void WebSocket2::operator()(const core::web::ClientSocket::Disconnected &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void WebSocket2::operator()(const core::web::ClientSocket::Ready &) {
  (*this)(ConnectionStatus::READY);
  subscribe();
}

void WebSocket2::operator()(const core::web::ClientSocket::Close &) {
}

void WebSocket2::operator()(const core::web::ClientSocket::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void WebSocket2::operator()(const core::web::ClientSocket::Text &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(const core::web::ClientSocket::Binary &binary) {
  if (inflate_.decode(binary.payload, inflate_buffer_, [&](auto &payload) {
        std::string_view message{
            reinterpret_cast<char const *>(std::data(payload)), std::size(payload)};
        log::info<5>(R"(message="{}")"sv, message);
        parse(message);
      })) {
  } else {
    log::fatal("Failed to decode message"sv);
  }
}

void WebSocket2::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void WebSocket2::subscribe(const std::span<std::string const> &symbols) {
  if (std::empty(symbols))
    return;
  if (shared_.api.has_funding_rate)
    subscribe(symbols, "public"sv, "funding_rate"sv);
}

void WebSocket2::subscribe(
    const std::span<std::string const> &symbols,
    const std::string_view &source,
    const std::string_view &theme) {
  assert(!std::empty(symbols));
  for (auto &symbol : symbols) {
    // auto id = ++request_id_;
    auto message = fmt::format(
        R"({{)"
        R"("op":"sub",)"
        R"("topic":"{}.{}.{}")"
        R"(}})"sv,
        source,
        symbol,
        theme);
    log::debug(R"(message="{}")"sv, message);
    connection_.send_text(message);
  }
}

void WebSocket2::send_pong(std::chrono::milliseconds timestamp) {
  auto message = fmt::format(
      R"({{)"
      R"("op":"pong",)"
      R"("ts":{})"
      R"(}})"sv,
      timestamp.count());
  // log::debug(R"(message="{}")"sv, message);
  connection_.send_text(message);
}

void WebSocket2::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      auto trace_info = server::create_trace_info();
      core::json::Buffer buffer(decode_buffer_);
      json::Parser2::dispatch(*this, message, buffer, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void WebSocket2::operator()(const server::Trace<json::Ping> &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    log::info<4>("trace_info={}, ping={}"sv, trace_info, ping);
    send_pong(ping.timestamp);
  });
}

void WebSocket2::operator()(const server::Trace<json::Close> &event) {
  profile_.close([&]() {
    auto &[trace_info, close] = event;
    log::warn("trace_info={}, close={}"sv, trace_info, close);
    connection_.close();
  });
}

void WebSocket2::operator()(const server::Trace<json::FundingRate> &event) {
  profile_.funding_rate([&]() {
    auto &[trace_info, funding_rate] = event;
    log::info<3>("trace_info={}, funding_rate={}"sv, trace_info, funding_rate);
    for (auto &item : funding_rate.data) {
      auto symbol = item.contract_code;
      Statistics statistics[] = {
          {
              .type = StatisticsType::FUNDING_RATE,
              .value = item.funding_rate,
              .begin_time_utc = {},
              .end_time_utc = {},
          },
          {
              .type = StatisticsType::FUNDING_RATE_PREDICTION,
              .value = item.estimated_rate,
              .begin_time_utc = {},
              .end_time_utc = {},
          },
      };
      const StatisticsUpdate statistics_update{
          .stream_id = stream_id_,
          .exchange = Flags::exchange(),
          .symbol = symbol,
          .statistics = statistics,
          .update_type = UpdateType::INCREMENTAL,
          .exchange_time_utc = utils::safe_cast(funding_rate.ts),
      };
      server::create_trace_and_dispatch(handler_, trace_info, statistics_update, true);
    }
  });
}

}  // namespace huobi_futures
}  // namespace roq
