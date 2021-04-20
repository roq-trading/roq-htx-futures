/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi/market_data.h"

#include <algorithm>

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/back_emplacer.h"
#include "roq/core/charconv.h"

#include "roq/core/tools/exception.h"

#include "roq/core/string_utils/string_builder.h"

#include "roq/core/metrics/factory.h"

#include "roq/huobi/flags.h"

using namespace roq::literals;

namespace roq {
namespace huobi {

namespace {
static const auto NAME = "md"_sv;
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
  };
}
}  // namespace

MarketData::MarketData(
    Handler &handler, core::io::Context &context, uint32_t stream_id, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(roq::format("{}:{}"_fmt, stream_id_, NAME)),
      connection_(
          *this,
          context,
          core::URI(Flags::ws_uri()),
          std::string_view{},  // query
          Flags::ws_ping_freq(),
          Flags::decode_buffer_size(),
          Flags::encode_buffer_size(),
          []() { return std::string(); }),
      decode_buffer_(Flags::decode_buffer_size()),
      request_id_(static_cast<uint64_t>(stream_id_) * 1000000u),  // scale (debugging)
      counter_{
          .disconnect = create_metrics(name_, "disconnect"_sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"_sv),
          .error = create_metrics(name_, "error"_sv),
          .result = create_metrics(name_, "result"_sv),
          .agg_trade = create_metrics(name_, "agg_trade"_sv),
          .trade = create_metrics(name_, "trade"_sv),
          .mini_ticker = create_metrics(name_, "mini_ticker"_sv),
          .book_ticker = create_metrics(name_, "book_ticker"_sv),
          .depth = create_metrics(name_, "depth"_sv),
          .depth_update = create_metrics(name_, "depth_update"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
          .heartbeat = create_metrics(name_, "heartbeat"_sv),
      },
      shared_(shared), download_({}, [this](auto state) { return download(state); }) {
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
      .write(profile_.result, metrics::PROFILE)
      .write(profile_.agg_trade, metrics::PROFILE)
      .write(profile_.trade, metrics::PROFILE)
      .write(profile_.mini_ticker, metrics::PROFILE)
      .write(profile_.book_ticker, metrics::PROFILE)
      .write(profile_.depth, metrics::PROFILE)
      .write(profile_.depth_update, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void MarketData::update_subscriptions(std::vector<std::string> &symbols) {
  assert(&symbols != &symbols_);
  auto max_size = Flags::ws_max_subscriptions_per_stream();
  auto offset = symbols_.size();
  if (max_size <= offset)
    return;
  if (symbols.empty())
    return;
  symbols_.reserve(max_size);
  auto length = std::min(max_size - offset, symbols.size());
  assert(length > 0u);
  for (size_t i = {}; i < length; ++i) {
    symbols_.emplace_back(symbols.back());
    symbols.pop_back();
  }
  assert(length == (symbols_.size() - offset));
  if (ready_)
    subscribe({&symbols_[offset], length});
}

void MarketData::operator()(const core::web::Socket::Connected &) {
}

void MarketData::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void MarketData::operator()(const core::web::Socket::Ready &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void MarketData::operator()(const core::web::Socket::Close &) {
}

void MarketData::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(trace_info, external_latency, handler_);
  latency_.ping.update(latency.sample);
}

void MarketData::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void MarketData::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    server::TraceInfo trace_info;
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"_fmt, stream_status);
    server::create_trace_and_dispatch(trace_info, stream_status, handler_);
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
  if (Flags::ws_subscribe_trade_details()) {
    subscribe_trade(symbols);
  } else {
    subscribe_agg_trade(symbols);
  }
  subscribe_mini_ticker(symbols);
  subscribe_book_ticker(symbols);
  subscribe_depth(symbols);
}

void MarketData::subscribe_agg_trade(const roq::span<std::string> &symbols) {
  assert(!symbols.empty());
  auto id = ++request_id_;
  auto message = roq::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}@aggTrade"],)"
      R"("id":{})"
      R"(}})"_fmt,
      roq::join(symbols, R"(@aggTrade",")"_sv),
      id);
  connection_.send_text(message);
}

void MarketData::subscribe_trade(const roq::span<std::string> &symbols) {
  assert(!symbols.empty());
  auto id = ++request_id_;
  auto message = roq::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}@trade"],)"
      R"("id":{})"
      R"(}})"_fmt,
      roq::join(symbols, R"(@trade",")"_sv),
      id);
  connection_.send_text(message);
}

void MarketData::subscribe_mini_ticker(const roq::span<std::string> &symbols) {
  assert(!symbols.empty());
  auto id = ++request_id_;
  auto message = roq::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}@miniTicker"],)"
      R"("id":{})"
      R"(}})"_fmt,
      roq::join(symbols, R"(@miniTicker",")"_sv),
      id);
  connection_.send_text(message);
}

void MarketData::subscribe_book_ticker(const roq::span<std::string> &symbols) {
  assert(!symbols.empty());
  auto id = ++request_id_;
  auto message = roq::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}@bookTicker"],)"
      R"("id":{})"
      R"(}})"_fmt,
      roq::join(symbols, R"(@bookTicker",")"_sv),
      id);
  connection_.send_text(message);
}

void MarketData::subscribe_depth(const roq::span<std::string> &symbols) {
  assert(!symbols.empty());
  auto stream = roq::format(
      R"(@depth{}@{}ms)"_fmt,
      Flags::ws_subscribe_depth_levels(),
      Flags::ws_subscribe_depth_freq().count());
  auto id = ++request_id_;
  auto separator = roq::format(R"({}",")"_fmt, stream);
  auto message = roq::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}{}"],)"
      R"("id":{})"
      R"(}})"_fmt,
      roq::join(symbols, separator),
      stream,
      id);
  connection_.send_text(message);
}

void MarketData::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      server::TraceInfo trace_info;
      core::json::Buffer buffer(decode_buffer_);
      json::MarketStreamParser::dispatch(*this, message, buffer, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"_fmt, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void MarketData::operator()(int32_t id, const json::Error &error) {
  profile_.error([&]() { log::warn("id={}, error={}"_fmt, id, error); });
}

void MarketData::operator()(int32_t id, const json::Result &result) {
  profile_.result([&]() { log::info("id={}, result={}"_fmt, id, result); });
}

void MarketData::operator()(const json::AggTrade &agg_trade, const server::TraceInfo &trace_info) {
  profile_.agg_trade([&]() {
    log::trace_3("agg_trade={}"_fmt, agg_trade);
    auto side = agg_trade.buyer_is_maker ? Side::BUY : Side::SELL;
    Trade trade{
        .side = side,
        .price = agg_trade.price,
        .quantity = agg_trade.quantity,
        .trade_id = {},
    };
    core::charconv::to_string(
        core::string_utils::string_builder(trade.trade_id), agg_trade.agg_trade_id);
    TradeSummary trade_summary{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = agg_trade.symbol,
        .trades = {&trade, 1u},
        .exchange_time_utc = agg_trade.event_time,
    };
    create_trace_and_dispatch(trace_info, trade_summary, handler_, true);
  });
}

void MarketData::operator()(const json::Trade &trade, const server::TraceInfo &trace_info) {
  profile_.trade([&]() {
    log::trace_3("trade={}"_fmt, trade);
    auto side = trade.buyer_is_maker ? Side::BUY : Side::SELL;
    Trade trade_{
        .side = side,
        .price = trade.price,
        .quantity = trade.quantity,
        .trade_id = {},
    };
    core::charconv::to_string(core::string_utils::string_builder(trade_.trade_id), trade.trade_id);
    TradeSummary trade_summary{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = trade.symbol,
        .trades = {&trade_, 1u},
        .exchange_time_utc = trade.event_time,
    };
    create_trace_and_dispatch(trace_info, trade_summary, handler_, true);
  });
}

void MarketData::operator()(
    const json::MiniTicker &mini_ticker, const server::TraceInfo &trace_info) {
  profile_.mini_ticker([&]() {
    log::trace_3("mini_ticker={}"_fmt, mini_ticker);
    Statistics statistics[] = {
        {.type = StatisticsType::HIGHEST_TRADED_PRICE, .value = mini_ticker.high_price},
        {.type = StatisticsType::LOWEST_TRADED_PRICE, .value = mini_ticker.low_price},
        {.type = StatisticsType::OPEN_PRICE, .value = mini_ticker.open_price},
        {.type = StatisticsType::CLOSE_PRICE, .value = mini_ticker.close_price},
    };
    StatisticsUpdate statistics_update{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = mini_ticker.symbol,
        .statistics = statistics,
        .snapshot = false,
        .exchange_time_utc = mini_ticker.event_time,
    };
    create_trace_and_dispatch(trace_info, statistics_update, handler_, true);
  });
}

void MarketData::operator()(
    const json::BookTicker &book_ticker, const server::TraceInfo &trace_info) {
  profile_.book_ticker([&]() {
    log::trace_3("book_ticker={}"_fmt, book_ticker);
    TopOfBook top_of_book{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = book_ticker.symbol,
        .layer{
            .bid_price = book_ticker.best_bid_price,
            .bid_quantity = book_ticker.best_bid_qty,
            .ask_price = book_ticker.best_ask_price,
            .ask_quantity = book_ticker.best_ask_qty,
        },
        .snapshot = false,
        .exchange_time_utc = {},
    };
    create_trace_and_dispatch(trace_info, top_of_book, handler_, true);
  });
}

void MarketData::operator()(
    const std::string_view &symbol, const json::Depth &depth, const server::TraceInfo &trace_info) {
  profile_.depth([&]() {
    log::trace_3(R"(symbol="{}", depth={})"_fmt, symbol, depth);
    core::back_emplacer bids(shared_.bids), asks(shared_.asks);
    for (auto &item : depth.bids)
      bids.emplace_back([&item](auto &result) { emplace(result, item); });
    for (auto &item : depth.asks)
      asks.emplace_back([&item](auto &result) { emplace(result, item); });
    if (!(bids.empty() && asks.empty())) {
      MarketByPriceUpdate market_by_price_update{
          .stream_id = stream_id_,
          .exchange = Flags::exchange(),
          .symbol = symbol,
          .bids = bids,
          .asks = asks,
          .snapshot = true,
          .exchange_time_utc = {},
      };
      create_trace_and_dispatch(trace_info, market_by_price_update, handler_, true);
    }
  });
}

void MarketData::operator()(
    const std::string_view &symbol,
    const json::DepthUpdate &depth_update,
    const server::TraceInfo &) {
  profile_.depth_update([&]() {
    log::trace_3(R"(symbol="{}", depth_update={})"_fmt, symbol, depth_update);
    // do nothing
    // XXX why?
  });
}

}  // namespace huobi
}  // namespace roq
