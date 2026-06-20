/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/gateway/market_data.hpp"

#include "roq/logging.hpp"

#include "roq/mask.hpp"

#include "roq/utils/charconv/to_string.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/htx_futures/protocol/json/map.hpp"
#include "roq/htx_futures/protocol/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "md"sv;

auto const SUPPORTS = Mask{
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
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
  auto uri = settings.ws.market_uri;
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = settings.net.disconnect_on_idle_timeout,
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

MarketData::MarketData(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      request_id_{static_cast<uint64_t>(stream_id_) * 1000000},  // scale (debugging)
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
          .total_bytes_received = create_metrics(shared.settings, name_, "total_bytes_received"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .error = create_metrics(shared.settings, name_, "error"sv),
          .subbed = create_metrics(shared.settings, name_, "subbed"sv),
          .bbo = create_metrics(shared.settings, name_, "bbo"sv),
          .depth = create_metrics(shared.settings, name_, "depth"sv),
          .trade = create_metrics(shared.settings, name_, "trade"sv),
          .detail = create_metrics(shared.settings, name_, "detail"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .heartbeat = create_metrics(shared.settings, name_, "heartbeat"sv),
      },
      shared_{shared}, inflate_{core::zlib::Inflate::GZIP_NO_HEADER} {
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

void MarketData::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      .write(counter_.total_bytes_received, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.error, metrics::Type::PROFILE)
      .write(profile_.subbed, metrics::Type::PROFILE)
      .write(profile_.bbo, metrics::Type::PROFILE)
      .write(profile_.depth, metrics::Type::PROFILE)
      .write(profile_.trade, metrics::Type::PROFILE)
      .write(profile_.detail, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

void MarketData::subscribe(size_t start_from) {
  if (ready()) {
    subscribe(shared_.symbols.get_slice(index_, start_from));
  }
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
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, external_latency);
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

void MarketData::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
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
  create_trace_and_dispatch(shared_.dispatcher, trace_info, stream_status);
}

void MarketData::subscribe(std::span<Symbol const> const &symbols) {
  if (std::empty(symbols)) {
    return;
  }
  subscribe(symbols, "market"sv, "bbo"sv);
  subscribe_with_data_type(symbols, "market"sv, shared_.api.market_data.market_depth, "incremental"sv);
  subscribe(symbols, "market"sv, "trade.detail"sv);
  subscribe(symbols, "market"sv, "detail"sv);
}

void MarketData::subscribe(std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme) {
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
    std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme, std::string_view const &data_type) {
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
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    try {
      TraceInfo trace_info;
      if (!protocol::json::Parser::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

void MarketData::operator()(Trace<protocol::json::Ping> const &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    send_pong(ping.timestamp);
  });
}

void MarketData::operator()(Trace<protocol::json::Error> const &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::warn("error={}"sv, error);
  });
}

void MarketData::operator()(Trace<protocol::json::Subbed> const &event) {
  profile_.subbed([&]() {
    auto &[trace_info, subbed] = event;
    log::info<1>("subbed={}"sv, subbed);
  });
}

void MarketData::operator()(Trace<protocol::json::BBO> const &event) {
  profile_.bbo([&]() {
    auto &[trace_info, bbo] = event;
    log::info<3>("bbo={}"sv, bbo);
    (*connection_).touch(trace_info.source_receive_time);
    auto symbol = protocol::json::extract_symbol(bbo.ch);
    auto &tick = bbo.tick;
    auto top_of_book = TopOfBook{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol,
        .layer{
            .bid_price = tick.bid.price,
            .bid_quantity = tick.bid.vol,
            .ask_price = tick.ask.price,
            .ask_quantity = tick.ask.vol,
        },
        .update_type = UpdateType::SNAPSHOT,
        .exchange_time_utc = bbo.ts,
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, top_of_book, true);
  });
}

void MarketData::operator()(Trace<protocol::json::Depth> const &event) {
  profile_.depth([&]() {
    auto &[trace_info, depth] = event;
    log::info<3>("depth={}"sv, depth);
    (*connection_).touch(trace_info.source_receive_time);
    auto symbol = protocol::json::extract_symbol(depth.ch);
    auto &tick = depth.tick;
    auto snapshot = tick.event == protocol::json::Event::SNAPSHOT;
    shared_.bids.clear();
    shared_.asks.clear();
    auto emplace_back = [](auto &result, auto &value) {
      auto mbp_update = MBPUpdate{
          .price = value.price,
          .quantity = value.vol,
          .implied_quantity = NaN,
          .number_of_orders = {},
          .update_action = {},
          .price_level = {},
      };
      result.emplace_back(std::move(mbp_update));
    };
    for (auto &item : tick.bids) {
      emplace_back(shared_.bids, item);
    }
    for (auto &item : tick.asks) {
      emplace_back(shared_.asks, item);
    }
    // XXX HANS validate checksum
    auto market_by_price_update = MarketByPriceUpdate{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol,
        .bids = shared_.bids,
        .asks = shared_.asks,
        .update_type = snapshot ? UpdateType::SNAPSHOT : UpdateType::INCREMENTAL,
        .exchange_time_utc = depth.ts,
        .exchange_sequence = {},
        .sending_time_utc = {},
        .price_precision = {},
        .quantity_precision = {},
        .checksum = {},
    };
    log::info<3>("market_by_price_update={}"sv, market_by_price_update);
    try {
      create_trace_and_dispatch(shared_.dispatcher, trace_info, market_by_price_update, true, shared_.final_bids, shared_.final_asks);
    } catch (BadState &) {
      // resubscribe_order_book_l2(symbol);
    }
  });
}

void MarketData::operator()(Trace<protocol::json::Trade> const &event) {
  profile_.trade([&]() {
    auto &[trace_info, trade] = event;
    log::info<3>("trade={}"sv, trade);
    (*connection_).touch(trace_info.source_receive_time);
    auto symbol = protocol::json::extract_symbol(trade.ch);
    auto &tick = trade.tick;
    shared_.trades.clear();
    auto emplace_back = [](auto &result, auto &value) {
      auto trade = Trade{
          .side = map(value.direction),
          .price = value.price,
          .quantity = value.amount,
          .trade_id = {},
          .taker_order_id = {},
          .maker_order_id = {},
      };
      utils::charconv::to_string(std::back_inserter(trade.trade_id), value.id);
      result.emplace_back(std::move(trade));
    };
    for (auto &item : tick.data) {
      emplace_back(shared_.trades, item);
    }
    auto trade_summary = TradeSummary{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol,
        .trades = shared_.trades,
        .exchange_time_utc = trade.ts,
        .exchange_sequence = trade.tick.id,
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, trade_summary, true);
  });
}

void MarketData::operator()(Trace<protocol::json::Detail> const &event) {
  profile_.detail([&]() {
    auto &[trace_info, detail] = event;
    log::info<3>("detail={}"sv, detail);
    (*connection_).touch(trace_info.source_receive_time);
    auto symbol = protocol::json::extract_symbol(detail.ch);
    auto &tick = detail.tick;
    auto statistics = std::array<Statistics, 5>{{
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
    }};
    auto statistics_update = StatisticsUpdate{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol,
        .statistics = statistics,
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = detail.ts,
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, statistics_update, true);
  });
}

void MarketData::operator()(Trace<protocol::json::EstimatedRate> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<protocol::json::PremiumIndex> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<protocol::json::Basis> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<protocol::json::Index> const &) {
  log::fatal("Unexpected"sv);
}

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
