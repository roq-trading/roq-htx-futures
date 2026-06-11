/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/gateway/web_socket_2.hpp"

#include "roq/mask.hpp"

#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/htx_futures/protocol/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "ws2"sv;

auto const SUPPORTS = Mask{
    SupportType::STATISTICS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 1;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.ws.order_uri;
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = {},
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.ping_freq,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::socket::Client::create(handler, context, config, []() { return std::string(); });
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

WebSocket2::WebSocket2(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
          .total_bytes_received = create_metrics(shared.settings, name_, "total_bytes_received"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .close = create_metrics(shared.settings, name_, "close"sv),
          .error = create_metrics(shared.settings, name_, "error"sv),
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .sub = create_metrics(shared.settings, name_, "sub"sv),
          .funding_rate = create_metrics(shared.settings, name_, "funding_rate"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
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

void WebSocket2::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(counter_.total_bytes_received, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.close, metrics::Type::PROFILE)
      .write(profile_.error, metrics::Type::PROFILE)
      .write(profile_.ping, metrics::Type::PROFILE)
      .write(profile_.sub, metrics::Type::PROFILE)
      .write(profile_.funding_rate, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

void WebSocket2::subscribe(size_t start_from) {
  if (ready()) {
    subscribe(shared_.symbols.get_slice(index_, start_from));
  }
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

void WebSocket2::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = {},
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::WS,
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
  create_trace_and_dispatch(handler_, trace_info, stream_status);
}

void WebSocket2::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols)) {
    return;
  }
  subscribe(symbols, "public"sv, "funding_rate"sv);
}

void WebSocket2::subscribe(std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme) {
  assert(!std::empty(symbols));
  for (auto &symbol : symbols) {
    auto message = fmt::format(
        R"({{)"
        R"("op":"sub",)"
        R"("topic":"{}.{}.{}",)"
        R"("cid":"xxx")"
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
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void WebSocket2::parse(std::string_view const &message) {
  profile_.parse([&]() {
    log::info<5>(R"(message="{}")"sv, message);
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    try {
      TraceInfo trace_info;
      if (!protocol::json::Parser2::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

void WebSocket2::operator()(Trace<protocol::json::Close2> const &) {
  profile_.close([&]() {
    log::warn("Exchange requested connection closed"sv);
    (*connection_).close();
  });
}

void WebSocket2::operator()(Trace<protocol::json::Error2> const &) {
  profile_.close([&]() {
    log::warn("*** ERROR ***"sv);
    (*connection_).close();
  });
}

void WebSocket2::operator()(Trace<protocol::json::Ping> const &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    log::info<4>("ping={}"sv, ping);
    send_pong(ping.timestamp);
  });
}

void WebSocket2::operator()(Trace<protocol::json::Auth> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(Trace<protocol::json::Sub> const &event) {
  profile_.sub([&]() {
    auto &[trace_info, sub] = event;
    if (sub.err_code != 0) {
      log::error(R"(Subscription failed: code={}, msg="{}")"sv, sub.err_code, sub.err_msg);
    }
  });
}

void WebSocket2::operator()(Trace<protocol::json::FundingRate> const &event) {
  profile_.funding_rate([&]() {
    auto &[trace_info, funding_rate] = event;
    log::info<3>("funding_rate={}"sv, funding_rate);
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
          .exchange = shared_.settings.exchange,
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

void WebSocket2::operator()(Trace<protocol::json::Accounts> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(Trace<protocol::json::Positions> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(Trace<protocol::json::MatchOrders> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(Trace<protocol::json::Orders> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(Trace<protocol::json::AccountsCross> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(Trace<protocol::json::PositionsCross> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(Trace<protocol::json::MatchOrdersCross> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket2::operator()(Trace<protocol::json::OrdersCross> const &) {
  log::fatal("Unexpected"sv);
}

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
