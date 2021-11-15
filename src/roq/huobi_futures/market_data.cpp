/* Copyright (c) 2017-2021, Hans Erik Thrane */

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
          .error = create_metrics(name_, "error"sv),
          .result = create_metrics(name_, "result"sv),
          .agg_trade = create_metrics(name_, "agg_trade"sv),
          .trade = create_metrics(name_, "trade"sv),
          .mini_ticker = create_metrics(name_, "mini_ticker"sv),
          .book_ticker = create_metrics(name_, "book_ticker"sv),
          .depth = create_metrics(name_, "depth"sv),
          .depth_update = create_metrics(name_, "depth_update"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
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
  assert(length > 0);
  for (size_t i = {}; i < length; ++i) {
    symbols_.emplace_back(symbols.back());
    symbols.pop_back();
  }
  assert(length == (symbols_.size() - offset));
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

void MarketData::operator()(const core::web::ClientSocket::Text &text) {
  parse(text.payload);
}

void MarketData::operator()(const core::web::ClientSocket::Binary &) {
  log::fatal("Unexpected"sv);
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
  auto message = fmt::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}@aggTrade"],)"
      R"("id":{})"
      R"(}})"sv,
      fmt::join(symbols, R"(@aggTrade",")"sv),
      id);
  connection_.send_text(message);
}

void MarketData::subscribe_trade(const roq::span<std::string> &symbols) {
  assert(!symbols.empty());
  auto id = ++request_id_;
  auto message = fmt::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}@trade"],)"
      R"("id":{})"
      R"(}})"sv,
      fmt::join(symbols, R"(@trade",")"sv),
      id);
  connection_.send_text(message);
}

void MarketData::subscribe_mini_ticker(const roq::span<std::string> &symbols) {
  assert(!symbols.empty());
  auto id = ++request_id_;
  auto message = fmt::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}@miniTicker"],)"
      R"("id":{})"
      R"(}})"sv,
      fmt::join(symbols, R"(@miniTicker",")"sv),
      id);
  connection_.send_text(message);
}

void MarketData::subscribe_book_ticker(const roq::span<std::string> &symbols) {
  assert(!symbols.empty());
  auto id = ++request_id_;
  auto message = fmt::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}@bookTicker"],)"
      R"("id":{})"
      R"(}})"sv,
      fmt::join(symbols, R"(@bookTicker",")"sv),
      id);
  connection_.send_text(message);
}

void MarketData::subscribe_depth(const roq::span<std::string> &symbols) {
  assert(!symbols.empty());
  auto stream = fmt::format(
      R"(@depth{}@{}ms)"sv,
      Flags::ws_subscribe_depth_levels(),
      std::chrono::duration_cast<std::chrono::milliseconds>(Flags::ws_subscribe_depth_freq())
          .count());
  auto id = ++request_id_;
  auto separator = fmt::format(R"({}",")"sv, stream);
  auto message = fmt::format(
      R"({{)"
      R"("method":"SUBSCRIBE",)"
      R"("params":["{}{}"],)"
      R"("id":{})"
      R"(}})"sv,
      fmt::join(symbols, separator),
      stream,
      id);
  connection_.send_text(message);
}

void MarketData::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      auto trace_info = server::create_trace_info();
      core::json::Buffer buffer(decode_buffer_);
      json::MarketStreamParser::dispatch(*this, message, buffer, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void MarketData::operator()(int32_t id, const json::Error &error) {
  profile_.error([&]() { log::warn("id={}, error={}"sv, id, error); });
}

void MarketData::operator()(int32_t id, const json::Result &result) {
  profile_.result([&]() { log::info("id={}, result={}"sv, id, result); });
}

void MarketData::operator()(const json::AggTrade &agg_trade, const server::TraceInfo &trace_info) {
  profile_.agg_trade([&]() {
    log::info<3>("agg_trade={}"sv, agg_trade);
    auto side = agg_trade.buyer_is_maker ? Side::BUY : Side::SELL;
    Trade trade{
        .side = side,
        .price = agg_trade.price,
        .quantity = agg_trade.quantity,
        .trade_id = {},
    };
    core::charconv::to_string(std::back_inserter(trade.trade_id), agg_trade.agg_trade_id);
    TradeSummary trade_summary{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = agg_trade.symbol,
        .trades = {&trade, 1},
        .exchange_time_utc = agg_trade.event_time,
    };
    create_trace_and_dispatch(handler_, trace_info, trade_summary, true);
  });
}

void MarketData::operator()(const json::Trade &trade, const server::TraceInfo &trace_info) {
  profile_.trade([&]() {
    log::info<3>("trade={}"sv, trade);
    auto side = trade.buyer_is_maker ? Side::BUY : Side::SELL;
    Trade trade_{
        .side = side,
        .price = trade.price,
        .quantity = trade.quantity,
        .trade_id = {},
    };
    core::charconv::to_string(std::back_inserter(trade_.trade_id), trade.trade_id);
    TradeSummary trade_summary{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = trade.symbol,
        .trades = {&trade_, 1},
        .exchange_time_utc = trade.event_time,
    };
    create_trace_and_dispatch(handler_, trace_info, trade_summary, true);
  });
}

void MarketData::operator()(
    const json::MiniTicker &mini_ticker, const server::TraceInfo &trace_info) {
  profile_.mini_ticker([&]() {
    log::info<3>("mini_ticker={}"sv, mini_ticker);
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
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = mini_ticker.event_time,
    };
    create_trace_and_dispatch(handler_, trace_info, statistics_update, true);
  });
}

void MarketData::operator()(
    const json::BookTicker &book_ticker, const server::TraceInfo &trace_info) {
  profile_.book_ticker([&]() {
    log::info<3>("book_ticker={}"sv, book_ticker);
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
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = {},
    };
    create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
  });
}

void MarketData::operator()(
    const std::string_view &symbol, const json::Depth &depth, const server::TraceInfo &trace_info) {
  profile_.depth([&]() {
    log::info<3>(R"(symbol="{}", depth={})"sv, symbol, depth);
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
          .update_type = UpdateType::SNAPSHOT,
          .exchange_time_utc = {},
          .exchange_sequence = {},
          .price_decimals = {},
          .quantity_decimals = {},
          .checksum = {},
      };
      create_trace_and_dispatch(handler_, trace_info, market_by_price_update, true, false);
    }
  });
}

void MarketData::operator()(
    const std::string_view &symbol,
    const json::DepthUpdate &depth_update,
    const server::TraceInfo &) {
  profile_.depth_update([&]() {
    log::info<3>(R"(symbol="{}", depth_update={})"sv, symbol, depth_update);
    // do nothing
    // XXX why?
  });
}

}  // namespace huobi_futures
}  // namespace roq
