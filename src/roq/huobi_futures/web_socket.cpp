/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/web_socket.hpp"

#include <algorithm>

#include "roq/mask.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/back_emplacer.hpp"
#include "roq/core/charconv.hpp"

#include "roq/core/tools/exception.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/huobi_futures/flags.hpp"

#include "roq/huobi_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

namespace {
const auto NAME = "ws"sv;
const Mask<SupportType> SUPPORTS{
    SupportType::STATISTICS,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::ws_index_uri();
  core::web::ClientSocket::Config config{
      .validate_certificate = server::Flags::tls_validate_certificate(),
      .uris = {&uri, 1},
      .query = {},
      .ping_frequency = Flags::ws_ping_freq(),
      .read_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
  };
  return core::web::ClientSocket{handler, context, config, []() { return std::string(); }};
}

template <typename T>
void emplace(Trade &result, const T &value) {
  new (&result) Trade{
      .side = json::map(value.direction),
      .price = value.price,
      .quantity = value.quantity,
      .trade_id = {},
  };
}

template <typename T>
void emplace(MBPUpdate &result, const T &value) {
  new (&result) MBPUpdate{
      .price = value.price,
      .quantity = value.vol,
      .implied_quantity = NaN,
      .price_level = {},
      .number_of_orders = {},
  };
}
}  // namespace

WebSocket::WebSocket(
    Handler &handler, core::io::Context &context, uint32_t stream_id, Shared &shared, size_t index)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      index_(index), connection_(create_connection(*this, context)),
      decode_buffer_(Flags::decode_buffer_size()),
      request_id_(static_cast<uint64_t>(stream_id_) * 1000000),  // scale (debugging)
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
          .ping = create_metrics(name_, "ping"sv),
          .error = create_metrics(name_, "error"sv),
          .subbed = create_metrics(name_, "subbed"sv),
          .estimated_rate = create_metrics(name_, "estimated_rate"sv),
          .premium_index = create_metrics(name_, "premium_index"sv),
          .basis = create_metrics(name_, "basis"sv),
          .index = create_metrics(name_, "index"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
      },
      shared_(shared), inflate_(core::zlib::Inflate::GZIP_NO_HEADER) {
}

void WebSocket::operator()(const Event<Start> &) {
  connection_.start();
}

void WebSocket::operator()(const Event<Stop> &) {
  connection_.stop();
}

void WebSocket::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void WebSocket::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.error, metrics::PROFILE)
      .write(profile_.subbed, metrics::PROFILE)
      .write(profile_.estimated_rate, metrics::PROFILE)
      .write(profile_.premium_index, metrics::PROFILE)
      .write(profile_.basis, metrics::PROFILE)
      .write(profile_.index, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void WebSocket::subscribe(size_t start_from) {
  if (ready())
    subscribe(shared_.symbols.get_slice(index_, start_from));
}

void WebSocket::operator()(const core::web::ClientSocket::Connected &) {
}

void WebSocket::operator()(const core::web::ClientSocket::Disconnected &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void WebSocket::operator()(const core::web::ClientSocket::Ready &) {
  (*this)(ConnectionStatus::READY);
  subscribe();
}

void WebSocket::operator()(const core::web::ClientSocket::Close &) {
}

void WebSocket::operator()(const core::web::ClientSocket::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void WebSocket::operator()(const core::web::ClientSocket::Text &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(const core::web::ClientSocket::Binary &binary) {
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

void WebSocket::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void WebSocket::subscribe(const std::span<Symbol const> &symbols) {
  if (std::empty(symbols))
    return;
  subscribe(symbols, "market"sv, "basis.1min.open"sv);
  if (shared_.api.has_estimated_rate)
    subscribe(symbols, "market"sv, "estimated_rate.1min"sv);
  if (shared_.api.has_index)
    subscribe(symbols, "market"sv, "index.1min"sv);
}

void WebSocket::subscribe(
    const std::span<Symbol const> &symbols,
    const std::string_view &source,
    const std::string_view &theme) {
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
    connection_.send_text(message);
  }
}

void WebSocket::send_pong(std::chrono::milliseconds timestamp) {
  auto message = fmt::format(
      R"({{)"
      R"("pong":{})"
      R"(}})"sv,
      timestamp.count());
  // log::debug(R"(message="{}")"sv, message);
  connection_.send_text(message);
}

void WebSocket::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      // log::debug("HERE {}"sv, message);
      auto trace_info = server::create_trace_info();
      core::json::Buffer buffer(decode_buffer_);
      if (json::Parser::dispatch(*this, message, buffer, trace_info)) {
      } else {
        log::warn(R"(Unable to parse message="{}")"sv, message);
      }
    } catch (...) {
      log::fatal(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void WebSocket::operator()(const Trace<json::Ping> &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    send_pong(ping.timestamp);
  });
}

void WebSocket::operator()(const Trace<json::Error> &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::warn("error={}"sv, error);
  });
}

void WebSocket::operator()(const Trace<json::Subbed> &event) {
  profile_.subbed([&]() {
    auto &[trace_info, subbed] = event;
    log::info<1>("subbed={}"sv, subbed);
  });
}

void WebSocket::operator()(const Trace<json::BBO> &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(const Trace<json::Depth> &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(const Trace<json::Trade> &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(const Trace<json::Detail> &) {
  log::fatal("Unexpected"sv);
}

void WebSocket::operator()(const Trace<json::EstimatedRate> &event) {
  profile_.estimated_rate([&]() {
    auto &[trace_info, estimated_rate] = event;
    log::info<3>("estimated_rate={}"sv, estimated_rate);
  });
}

void WebSocket::operator()(const Trace<json::PremiumIndex> &event) {
  profile_.premium_index([&]() {
    auto &[trace_info, premium_index] = event;
    log::info<3>("premium_index={}"sv, premium_index);
  });
}

void WebSocket::operator()(const Trace<json::Index> &event) {
  profile_.index([&]() {
    auto &[trace_info, index] = event;
    log::info<3>("index={}"sv, index);
    auto symbol = json::extract_symbol(index.ch);
    auto &tick = index.tick;
    Statistics statistics[] = {
        {
            .type = StatisticsType::INDEX_VALUE,
            .value = tick.close,
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
        .exchange_time_utc = utils::safe_cast(index.ts),
    };
    create_trace_and_dispatch(handler_, trace_info, statistics_update, true);
  });
}

void WebSocket::operator()(const Trace<json::Basis> &event) {
  profile_.basis([&]() {
    auto &[trace_info, basis] = event;
    log::info<3>("basis={}"sv, basis);
  });
}

}  // namespace huobi_futures
}  // namespace roq
