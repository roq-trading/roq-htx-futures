/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi/json/market_stream_parser.h"

#include <cctype>
#include <string>

#include "roq/compat.h"

#include "roq/huobi/json/field.h"
#include "roq/huobi/json/stream.h"

#include "roq/logging.h"

using namespace roq::literals;

namespace roq {
namespace huobi {
namespace json {

void MarketStreamParser::dispatch(
    MarketStreamParser::Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    const server::TraceInfo &trace_info) {
  int64_t id = -1;
  std::string symbol;  // allocating because we need uppercase
  auto stream = Stream::UNDEFINED;
  bool dispatched = false;
  for (int i = 0; i < 2 && !dispatched; ++i) {
    core::json::Parser parser(message);
    auto root = parser.root();
    for (auto [key, value] : std::get<core::json::object_t>(root)) {
      auto field = Field(key);
      switch (field) {
        case Field::UNDEFINED:
          log::fatal("Unexpected"_sv);
          break;
        case Field::UNKNOWN:
#if !defined(NDEBUG)
          log::fatal(R"(Unknown key="{}")"_fmt, key);
#endif
          // XXX CALLBACK ?????????????
          break;
        case Field::ERROR:
          if (id >= 0) {
            Error error(value);
            dispatched = true;
            handler(id, error);
          }
          break;
        case Field::RESULT:
          if (id >= 0) {
            Result result(value, buffer);
            dispatched = true;
            handler(id, result);
          }
          break;
        case Field::ID:
          id = std::get<decltype(id)>(value);
          break;
        case Field::STREAM: {
          // <symbol>@<stream>[@<freq>]
          auto full_name = std::get<std::string_view>(value);
          auto idx0 = full_name.find('@');  // <symbol>@<stream>
          if (ROQ_UNLIKELY(idx0 == full_name.npos))
            log::fatal(R"(Unexpected: name="{}")"_fmt, full_name);
          symbol = std::string_view(full_name.begin(), idx0);
          // note! convert to uppercase
          std::transform(
              symbol.begin(), symbol.end(), symbol.begin(), [](auto c) { return std::toupper(c); });
          auto idx1 = full_name.find('@', idx0 + 1);
          auto name = std::string_view(
              full_name.begin() + idx0 + 1,
              (idx1 == full_name.npos) ? full_name.size() - idx0 - 1 : idx1 - idx0 - 1);
          stream = Stream(name);
          break;
        }
        case Field::DATA:
          switch (stream) {
            case Stream::UNDEFINED:
              break;  // wait
            case Stream::UNKNOWN:
#if !defined(NDEBUG)
              log::fatal("Unexpected (unknown stream)"_sv);
#endif
              return;
            case Stream::AGG_TRADE: {
              AggTrade agg_trade(value);
              dispatched = true;
              handler(agg_trade, trace_info);
              break;
            }
            case Stream::TRADE: {
              Trade trade(value);
              dispatched = true;
              handler(trade, trace_info);
              break;
            }
            case Stream::MINI_TICKER: {
              MiniTicker mini_ticker(value);
              dispatched = true;
              handler(mini_ticker, trace_info);
              break;
            }
            case Stream::BOOK_TICKER: {
              BookTicker book_ticker(value);
              dispatched = true;
              handler(book_ticker, trace_info);
              break;
            }
            case Stream::DEPTH5:
            case Stream::DEPTH10:
            case Stream::DEPTH20: {
              assert(!symbol.empty());
              Depth depth(value, buffer);
              dispatched = true;
              handler(symbol, depth, trace_info);
              break;
            }
            case Stream::DEPTH: {
              assert(!symbol.empty());
              DepthUpdate depth_update(value, buffer);
              dispatched = true;
              handler(symbol, depth_update, trace_info);
              break;
            }
          }
          break;
      }
    }
  }
  if (dispatched)
    return;
  log::warn(R"(message="{}")"_fmt, message);
  log::fatal("Unexpected"_sv);
}

}  // namespace json
}  // namespace huobi
}  // namespace roq
