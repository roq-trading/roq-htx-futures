/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/huobi_futures/web_socket.hpp"

#include <algorithm>

#include "roq/mask.hpp"

#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/core/tools/exception.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/huobi_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

// === CONSTANTS ===

namespace {
auto const NAME = "ws"sv;

auto const SUPPORTS = Mask{
    SupportType::STATISTICS,
};
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.ws.index_uri;
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
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.ping_freq,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::socket::Client::create(handler, context, config, []() -> std::string { return {}; });
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

WebSocket::WebSocket(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_(shared.settings.misc.decode_buffer_size), request_id_{static_cast<uint64_t>(stream_id_) * 1000000},  // scale (debugging)
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
          .total_bytes_received = create_metrics(shared.settings, name_, "total_bytes_received"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .error = create_metrics(shared.settings, name_, "error"sv),
          .subbed = create_metrics(shared.settings, name_, "subbed"sv),
          .estimated_rate = create_metrics(shared.settings, name_, "estimated_rate"sv),
          .premium_index = create_metrics(shared.settings, name_, "premium_index"sv),
          .basis = create_metrics(shared.settings, name_, "basis"sv),
          .index = create_metrics(shared.settings, name_, "index"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .heartbeat = create_metrics(shared.settings, name_, "heartbeat"sv),
      },
      shared_{shared}, inflate_{core::zlib::Inflate::GZIP_NO_HEADER} {
}

void WebSocket::operator()(Event<Start> const &) {
  (*connection_).start();
}

void WebSocket::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void WebSocket::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
}

void WebSocket::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.error, metrics::Type::PROFILE)
      .write(profile_.subbed, metrics::Type::PROFILE)
      .write(profile_.estimated_rate, metrics::Type::PROFILE)
      .write(profile_.premium_index, metrics::Type::PROFILE)
      .write(profile_.basis, metrics::Type::PROFILE)
      .write(profile_.index, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

void WebSocket::subscribe(size_t start_from) {
  if (ready())
    subscribe(shared_.symbols.get_slice(index_, start_from));
}

void WebSocket::operator()(web::socket::Client::Connected const &) {
}

void WebSocket::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void WebSocket::operator()(web::socket::Client::Ready const &) {
  (*this)(ConnectionStatus::READY);
  subscribe();
}

void WebSocket::operator()(web::socket::Client::Close const &) {
}

void WebSocket::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void WebSocket::operator()(web::socket::Client::Text const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(web::socket::Client::Binary const &binary) {
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

void WebSocket::operator()(ConnectionStatus status) {
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

void WebSocket::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols))
    return;
  subscribe(symbols, "market"sv, "basis.1min.open"sv);
  if (shared_.api.has_estimated_rate)
    subscribe(symbols, "market"sv, "estimated_rate.1min"sv);
  if (shared_.api.has_index)
    subscribe(symbols, "market"sv, "index.1min"sv);
}

void WebSocket::subscribe(std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme) {
  assert(!std::empty(symbols));
  for (auto &symbol : symbols) {
    auto id = ++request_id_;
    auto message = fmt::format(
        R"({{)"
        R"("sub":"{}.{}.{}",)"
        R"("id":"{}")"
        R"(}})"sv,
        source,
        symbol,
        theme,
        id);
    log::debug(R"(message="{}")"sv, message);
    (*connection_).send_text(message);
  }
}

void WebSocket::send_pong(std::chrono::milliseconds timestamp) {
  auto message = fmt::format(
      R"({{)"
      R"("pong":{})"
      R"(}})"sv,
      timestamp.count());
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void WebSocket::parse(std::string_view const &message) {
  profile_.parse([&]() {
    try {
      // log::debug("HERE {}"sv, message);
      TraceInfo trace_info;
      if (json::Parser::dispatch(*this, message, decode_buffer_, trace_info)) {
      } else {
        log::warn(R"(Unable to parse message="{}")"sv, message);
      }
    } catch (...) {
      log::fatal(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void WebSocket::operator()(Trace<json::Ping> const &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    send_pong(ping.timestamp);
  });
}

void WebSocket::operator()(Trace<json::Error> const &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::warn("error={}"sv, error);
  });
}

void WebSocket::operator()(Trace<json::Subbed> const &event) {
  profile_.subbed([&]() {
    auto &[trace_info, subbed] = event;
    log::info<1>("subbed={}"sv, subbed);
  });
}

void WebSocket::operator()(Trace<json::BBO> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::Depth> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::Trade> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::Detail> const &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(Trace<json::EstimatedRate> const &event) {
  profile_.estimated_rate([&]() {
    auto &[trace_info, estimated_rate] = event;
    log::info<3>("estimated_rate={}"sv, estimated_rate);
  });
}

void WebSocket::operator()(Trace<json::PremiumIndex> const &event) {
  profile_.premium_index([&]() {
    auto &[trace_info, premium_index] = event;
    log::info<3>("premium_index={}"sv, premium_index);
  });
}

void WebSocket::operator()(Trace<json::Index> const &event) {
  profile_.index([&]() {
    auto &[trace_info, index] = event;
    log::info<3>("index={}"sv, index);
    auto symbol = json::extract_symbol(index.ch);
    auto &tick = index.tick;
    auto statistics = Statistics{
        .type = StatisticsType::INDEX_VALUE,
        .value = tick.close,
        .begin_time_utc = {},
        .end_time_utc = {},
    };
    auto statistics_update = StatisticsUpdate{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol,
        .statistics = {&statistics, 1u},
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = index.ts,
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(handler_, trace_info, statistics_update, true);
  });
}

void WebSocket::operator()(Trace<json::Basis> const &event) {
  profile_.basis([&]() {
    auto &[trace_info, basis] = event;
    log::info<3>("basis={}"sv, basis);
  });
}

}  // namespace huobi_futures
}  // namespace roq
