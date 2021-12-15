/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/market_data.h"

#include <algorithm>

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/back_emplacer.h"
#include "roq/core/charconv.h"

#include "roq/core/tools/exception.h"

#include "roq/core/metrics/factory.h"

#include "roq/huobi_futures/flags.h"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

namespace {
static const auto NAME = "md"sv;
static const auto SUPPORTS = utils::Mask{
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
    SupportType::STATISTICS,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

template <typename T>
void emplace(MBPUpdate &result, const T &value) {
  new (&result) MBPUpdate{
      .price = value.price,
      .quantity = value.qty,
      .implied_quantity = NaN,
      .price_level = {},
      .number_of_orders = {},
  };
}
}  // namespace

MarketData::MarketData(
    Handler &handler, core::io::Context &context, uint32_t stream_id, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      connection_(
          *this,
          context,
          core::URI(Flags::ws_market_uri()),
          {},  // query
          Flags::ws_ping_freq(),
          Flags::decode_buffer_size(),
          Flags::encode_buffer_size(),
          []() { return std::string(); }),
      decode_buffer_(Flags::decode_buffer_size()),
      request_id_(static_cast<uint64_t>(stream_id_) * 1000000),  // scale (debugging)
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
          .ping = create_metrics(name_, "ping"sv),
          .error = create_metrics(name_, "error"sv),
          .bbo = create_metrics(name_, "bbo"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
      },
      shared_(shared), download_({}, [this](auto state) { return download(state); }),
      inflate_(core::zlib::Inflate::GZIP_NO_HEADER) {
}

bool MarketData::ready() const {
  return connection_.ready();
}

void MarketData::operator()(const Event<Start> &) {
  connection_.start();
}

void MarketData::operator()(const Event<Stop> &) {
  connection_.stop();
}

void MarketData::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void MarketData::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.error, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void MarketData::update_subscriptions(std::vector<std::string> &symbols) {
  assert(&symbols != &symbols_);
  auto max_size = Flags::ws_max_subscriptions_per_stream();
  auto offset = std::size(symbols_);
  if (max_size <= offset)
    return;
  if (std::empty(symbols))
    return;
  symbols_.reserve(max_size);
  auto length = std::min(max_size - offset, std::size(symbols));
  assert(length > 0);
  for (size_t i = 0; i < length; ++i) {
    symbols_.emplace_back(symbols.back());
    symbols.pop_back();
  }
  assert(length == (std::size(symbols_) - offset));
  if (ready_)
    subscribe({&symbols_[offset], length});
}

void MarketData::operator()(const core::web::ClientSocket::Connected &) {
}

void MarketData::operator()(const core::web::ClientSocket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void MarketData::operator()(const core::web::ClientSocket::Ready &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void MarketData::operator()(const core::web::ClientSocket::Close &) {
}

void MarketData::operator()(const core::web::ClientSocket::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void MarketData::operator()(const core::web::ClientSocket::Text &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(const core::web::ClientSocket::Binary &binary) {
  if (inflate_.decode(binary.payload, inflate_buffer_, [&](auto &payload) {
        std::string_view message{
            reinterpret_cast<char const *>(std::data(payload)), std::size(payload)};
        parse(message);
      })) {
  } else {
    log::fatal("Failed to decode message"sv);
  }
}

void MarketData::operator()(ConnectionStatus status) {
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

uint32_t MarketData::download(MarketDataState state) {
  switch (state) {
    case MarketDataState::UNDEFINED:
      assert(false);
      break;
    case MarketDataState::SUBSCRIBE:
      subscribe(symbols_);
      return {};
    case MarketDataState::DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return {};
  }
  assert(false);
  return {};
}

void MarketData::subscribe(const roq::span<std::string> &symbols) {
  if (std::empty(symbols))
    return;
  subscribe(symbols_, "bbo"sv);
}

void MarketData::subscribe(const roq::span<std::string> &symbols, const std::string_view &theme) {
  assert(!std::empty(symbols));
  for (auto &symbol : symbols) {
    auto id = ++request_id_;
    auto message = fmt::format(
        R"({{)"
        R"("sub":"market.{}.{}",)"
        R"("id":"{}")"
        R"(}})"sv,
        symbol,
        theme,
        id);
    log::debug(R"(message="{}")"sv, message);
    connection_.send_text(message);
  }
}

void MarketData::send_pong(std::chrono::milliseconds timestamp) {
  auto message = fmt::format(
      R"({{)"
      R"("pong":{})"
      R"(}})"sv,
      timestamp.count());
  log::debug(R"(message="{}")"sv, message);
  connection_.send_text(message);
}

void MarketData::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      log::debug("HERE {}"sv, message);
      auto trace_info = server::create_trace_info();
      core::json::Buffer buffer(decode_buffer_);
      if (json::Parser::dispatch(*this, message, buffer, trace_info)) {
      } else {
        log::warn(R"(Unable to parse message="{}")"sv, message);
      }
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void MarketData::operator()(const server::Trace<json::Ping> &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    send_pong(ping.timestamp);
  });
}

void MarketData::operator()(const server::Trace<json::Error> &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::warn("error={}"sv, error);
  });
}

void MarketData::operator()(const server::Trace<json::BBO> &event) {
  profile_.bbo([&]() {
    auto &[trace_info, bbo] = event;
    log::debug("bbo={}"sv, bbo);
  });
}

}  // namespace huobi_futures
}  // namespace roq
