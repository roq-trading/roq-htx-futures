/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/market_data.hpp"

#include <algorithm>

#include "roq/mask.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/back_emplacer.hpp"
#include "roq/core/charconv.hpp"

#include "roq/core/tools/exception.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/socket/client_factory.hpp"

#include "roq/huobi_futures/flags.hpp"

#include "roq/huobi_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

namespace {
auto const NAME = "md"sv;
const Mask SUPPORTS{
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
    SupportType::STATISTICS,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(std::string_view const &group, std::string_view const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::ws_market_uri();
  web::socket::Client::Config config{
      .always_reconnect = true,
      .connection_timeout = server::Flags::net_connection_timeout(),
      .disconnect_on_idle_timeout = server::Flags::net_disconnect_on_idle_timeout(),
      .validate_certificate = server::Flags::net_tls_validate_certificate(),
      .uris = {&uri, 1},
      .query = {},
      .ping_frequency = Flags::ws_ping_freq(),
      .read_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
  };
  return web::socket::ClientFactory::create(handler, context, config, []() { return std::string(); });
}

template <typename T>
void emplace(Trade &result, T const &value) {
  new (&result) Trade{
      .side = json::map(value.direction),
      .price = value.price,
      .quantity = value.amount,
      .trade_id = {},
      .taker_order_id = {},
      .maker_order_id = {},
  };
  core::charconv::to_string(std::back_inserter(result.trade_id), value.id);
}

template <typename T>
void emplace(MBPUpdate &result, T const &value) {
  new (&result) MBPUpdate{
      .price = value.price,
      .quantity = value.vol,
      .implied_quantity = NaN,
      .number_of_orders = {},
      .update_action = {},
      .price_level = {},
  };
}
}  // namespace

MarketData::MarketData(Handler &handler, io::Context &context, uint32_t stream_id, Shared &shared, size_t index)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)), index_(index),
      connection_(create_connection(*this, context)), decode_buffer_(Flags::decode_buffer_size()),
      request_id_(static_cast<uint64_t>(stream_id_) * 1000000),  // scale (debugging)
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
          .total_bytes_received = create_metrics(name_, "total_bytes_received"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
          .ping = create_metrics(name_, "ping"sv),
          .error = create_metrics(name_, "error"sv),
          .subbed = create_metrics(name_, "subbed"sv),
          .bbo = create_metrics(name_, "bbo"sv),
          .depth = create_metrics(name_, "depth"sv),
          .trade = create_metrics(name_, "trade"sv),
          .detail = create_metrics(name_, "detail"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
      },
      shared_(shared), inflate_(core::zlib::Inflate::GZIP_NO_HEADER) {
}

void MarketData::operator()(Event<Start> const &) {
  (*connection_).start();
}

void MarketData::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void MarketData::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
}

void MarketData::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      .write(counter_.total_bytes_received, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      .write(profile_.error, metrics::PROFILE)
      .write(profile_.subbed, metrics::PROFILE)
      .write(profile_.bbo, metrics::PROFILE)
      .write(profile_.depth, metrics::PROFILE)
      .write(profile_.trade, metrics::PROFILE)
      .write(profile_.detail, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void MarketData::subscribe(size_t start_from) {
  if (ready())
    subscribe(shared_.symbols.get_slice(index_, start_from));
}

void MarketData::operator()(web::socket::Client::Connected const &) {
}

void MarketData::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void MarketData::operator()(web::socket::Client::Ready const &) {
  (*this)(ConnectionStatus::READY);
  subscribe();
}

void MarketData::operator()(web::socket::Client::Close const &) {
}

void MarketData::operator()(web::socket::Client::Latency const &latency) {
  auto trace_info = server::create_trace_info();
  const ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void MarketData::operator()(web::socket::Client::Text const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(web::socket::Client::Binary const &binary) {
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

void MarketData::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    const StreamStatus stream_status{
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

void MarketData::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols))
    return;
  subscribe(symbols, "market"sv, "bbo"sv);
  subscribe_with_data_type(symbols, "market"sv, shared_.api.market_depth, "incremental"sv);
  subscribe(symbols, "market"sv, "trade.detail"sv);
  subscribe(symbols, "market"sv, "detail"sv);
}

void MarketData::subscribe(
    std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme) {
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

void MarketData::subscribe_with_data_type(
    std::span<Symbol const> const &symbols,
    std::string_view const &source,
    std::string_view const &theme,
    std::string_view const &data_type) {
  assert(!std::empty(symbols));
  for (auto &symbol : symbols) {
    auto id = ++request_id_;
    auto message = fmt::format(
        R"({{)"
        R"("sub":"{}.{}.{}",)"
        R"("data_type":"{}",)"
        R"("id":"{}")"
        R"(}})"sv,
        source,
        symbol,
        theme,
        data_type,
        id);
    log::debug(R"(message="{}")"sv, message);
    (*connection_).send_text(message);
  }
}

void MarketData::send_pong(std::chrono::milliseconds timestamp) {
  auto message = fmt::format(
      R"({{)"
      R"("pong":{})"
      R"(}})"sv,
      timestamp.count());
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void MarketData::parse(std::string_view const &message) {
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

void MarketData::operator()(Trace<json::Ping> const &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    send_pong(ping.timestamp);
  });
}

void MarketData::operator()(Trace<json::Error> const &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::warn("error={}"sv, error);
  });
}

void MarketData::operator()(Trace<json::Subbed> const &event) {
  profile_.subbed([&]() {
    auto &[trace_info, subbed] = event;
    log::info<1>("subbed={}"sv, subbed);
  });
}

void MarketData::operator()(Trace<json::BBO> const &event) {
  profile_.bbo([&]() {
    auto &[trace_info, bbo] = event;
    log::info<3>("bbo={}"sv, bbo);
    (*connection_).touch(trace_info.source_receive_time);
    auto symbol = json::extract_symbol(bbo.ch);
    auto &tick = bbo.tick;
    const TopOfBook top_of_book{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .layer{
            .bid_price = tick.bid.price,
            .bid_quantity = tick.bid.vol,
            .ask_price = tick.ask.price,
            .ask_quantity = tick.ask.vol,
        },
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = utils::safe_cast(bbo.ts),
        .exchange_sequence = {},
    };
    create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
  });
}

void MarketData::operator()(Trace<json::Depth> const &event) {
  profile_.depth([&]() {
    auto &[trace_info, depth] = event;
    log::info<3>("depth={}"sv, depth);
    (*connection_).touch(trace_info.source_receive_time);
    auto symbol = json::extract_symbol(depth.ch);
    auto &tick = depth.tick;
    auto snapshot = tick.event == json::Event::SNAPSHOT;
    core::back_emplacer bids(shared_.bids), asks(shared_.asks);
    for (auto &item : tick.bids)
      bids.emplace_back([&item](auto &result) { emplace(result, item); });
    for (auto &item : tick.asks)
      asks.emplace_back([&item](auto &result) { emplace(result, item); });
    // XXX HANS validate checksum
    const MarketByPriceUpdate market_by_price_update{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .bids = bids,
        .asks = asks,
        .update_type = snapshot ? UpdateType::SNAPSHOT : UpdateType::INCREMENTAL,
        .exchange_time_utc = utils::safe_cast(depth.ts),
        .exchange_sequence = {},
        .price_decimals = {},
        .quantity_decimals = {},
        .checksum = {},
    };
    log::info<3>("market_by_price_update={}"sv, market_by_price_update);
    try {
      create_trace_and_dispatch(handler_, trace_info, market_by_price_update, true, false);
    } catch (BadState &) {
      // resubscribe_order_book_l2(symbol);
    }
  });
}

void MarketData::operator()(Trace<json::Trade> const &event) {
  profile_.trade([&]() {
    auto &[trace_info, trade] = event;
    log::info<3>("trade={}"sv, trade);
    (*connection_).touch(trace_info.source_receive_time);
    auto symbol = json::extract_symbol(trade.ch);
    auto &tick = trade.tick;
    core::back_emplacer trades(shared_.trades);
    for (auto &item : tick.data)
      trades.emplace_back([&item](auto &result) { emplace(result, item); });
    const TradeSummary trade_summary{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .trades = trades,
        .exchange_time_utc = utils::safe_cast(trade.ts),
        .exchange_sequence = trade.tick.id,
    };
    create_trace_and_dispatch(handler_, trace_info, trade_summary, true);
  });
}

void MarketData::operator()(Trace<json::Detail> const &event) {
  profile_.detail([&]() {
    auto &[trace_info, detail] = event;
    log::info<3>("detail={}"sv, detail);
    (*connection_).touch(trace_info.source_receive_time);
    auto symbol = json::extract_symbol(detail.ch);
    auto &tick = detail.tick;
    Statistics statistics[] = {
        {
            .type = StatisticsType::OPEN_PRICE,
            .value = tick.open,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::HIGHEST_TRADED_PRICE,
            .value = tick.high,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::LOWEST_TRADED_PRICE,
            .value = tick.low,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::CLOSE_PRICE,
            .value = tick.close,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::TRADE_VOLUME,
            .value = tick.vol,  // note! not sure...  (amount? count?)
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
        .exchange_time_utc = utils::safe_cast(detail.ts),
    };
    create_trace_and_dispatch(handler_, trace_info, statistics_update, true);
  });
}

void MarketData::operator()(Trace<json::EstimatedRate> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::PremiumIndex> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::Basis> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::Index> const &) {
  log::fatal("Unexpected"sv);
}

}  // namespace huobi_futures
}  // namespace roq
