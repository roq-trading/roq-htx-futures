/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/huobi_futures/web_socket_2.hpp"

#include "roq/mask.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/socket/client_factory.hpp"

#include "roq/huobi_futures/flags.hpp"

#include "roq/huobi_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

// === CONSTANTS ===

namespace {
auto const NAME = "ws2"sv;

auto const SUPPORTS = Mask{
    SupportType::STATISTICS,
};
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::ws_order_uri();
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .validate_certificate = server::Flags::net_tls_validate_certificate(),
      // connection manager
      .connection_timeout = server::Flags::net_connection_timeout(),
      .disconnect_on_idle_timeout = {},
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = Flags::ws_ping_freq(),
      // implementation
      .decode_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
  };
  return web::socket::ClientFactory::create(handler, context, config, []() { return std::string(); });
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(auto const &group, auto const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

// === IMPLEMENTATION ===

WebSocket2::WebSocket2(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index},
      connection_{create_connection(*this, context)}, decode_buffer_{Flags::decode_buffer_size()},
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
          .total_bytes_received = create_metrics(name_, "total_bytes_received"sv),
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
      shared_{shared}, inflate_{core::zlib::Inflate::GZIP_NO_HEADER} {
}

void WebSocket2::operator()(Event<Start> const &) {
  (*connection_).start();
}

void WebSocket2::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void WebSocket2::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
}

void WebSocket2::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      .write(counter_.total_bytes_received, metrics::COUNTER)
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

void WebSocket2::operator()(web::socket::Client::Connected const &) {
}

void WebSocket2::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void WebSocket2::operator()(web::socket::Client::Ready const &) {
  (*this)(ConnectionStatus::READY);
  subscribe();
}

void WebSocket2::operator()(web::socket::Client::Close const &) {
}

void WebSocket2::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void WebSocket2::operator()(web::socket::Client::Text const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(web::socket::Client::Binary const &binary) {
  if (inflate_.decode(binary.payload, inflate_buffer_, [&](auto &payload) {
        std::string_view message{reinterpret_cast<char const *>(std::data(payload)), std::size(payload)};
        log::info<5>(R"(message="{}")"sv, message);
        parse(message);
      })) {
  } else {
    log::fatal("Failed to decode message"sv);
  }
  counter_.total_bytes_received.update((*connection_).total_bytes_received());
}

void WebSocket2::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::WS,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void WebSocket2::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols))
    return;
  if (shared_.api.has_funding_rate)
    subscribe(symbols, "public"sv, "funding_rate"sv);
}

void WebSocket2::subscribe(
    std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme) {
  assert(!std::empty(symbols));
  for (auto &symbol : symbols) {
    auto message = fmt::format(
        R"({{)"
        R"("op":"sub",)"
        R"("topic":"{}.{}.{}")"
        R"(}})"sv,
        source,
        symbol,
        theme);
    log::debug(R"(message="{}")"sv, message);
    (*connection_).send_text(message);
  }
}

void WebSocket2::send_pong(std::chrono::milliseconds timestamp) {
  auto message = fmt::format(
      R"({{)"
      R"("op":"pong",)"
      R"("ts":{})"
      R"(}})"sv,
      timestamp.count());
  log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void WebSocket2::parse(std::string_view const &message) {
  profile_.parse([&]() {
    try {
      TraceInfo trace_info;
      core::json::Buffer buffer{decode_buffer_};
      json::Parser2::dispatch(*this, message, buffer, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void WebSocket2::operator()(Trace<json::Ping> const &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    log::info<4>("trace_info={}, ping={}"sv, trace_info, ping);
    send_pong(ping.timestamp);
  });
}

void WebSocket2::operator()(Trace<json::Close> const &event) {
  profile_.close([&]() {
    auto &[trace_info, close] = event;
    log::warn("trace_info={}, close={}"sv, trace_info, close);
    (*connection_).close();
  });
}

void WebSocket2::operator()(Trace<json::FundingRate> const &event) {
  profile_.funding_rate([&]() {
    auto &[trace_info, funding_rate] = event;
    log::info<3>("trace_info={}, funding_rate={}"sv, trace_info, funding_rate);
    for (auto &item : funding_rate.data) {
      auto symbol = item.contract_code;
      auto statistics = std::array<Statistics, 2>{{
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
      }};
      auto statistics_update = StatisticsUpdate{
          .stream_id = stream_id_,
          .exchange = Flags::exchange(),
          .symbol = symbol,
          .statistics = statistics,
          .update_type = UpdateType::INCREMENTAL,
          .exchange_time_utc = funding_rate.ts,
          .exchange_sequence = {},
          .sending_time_utc = {},
      };
      create_trace_and_dispatch(handler_, trace_info, statistics_update, true);
    }
  });
}

}  // namespace huobi_futures
}  // namespace roq
