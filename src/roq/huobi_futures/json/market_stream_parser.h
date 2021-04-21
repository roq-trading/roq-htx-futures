/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/buffer.h"

#include "roq/server.h"

#include "roq/huobi_futures/json/error.h"
#include "roq/huobi_futures/json/result.h"

#include "roq/huobi_futures/json/agg_trade.h"
#include "roq/huobi_futures/json/book_ticker.h"
#include "roq/huobi_futures/json/depth.h"
#include "roq/huobi_futures/json/depth_update.h"
#include "roq/huobi_futures/json/mini_ticker.h"
#include "roq/huobi_futures/json/trade.h"

namespace roq {
namespace huobi_futures {
namespace json {

struct MarketStreamParser final {
  struct Handler {
    // response

    virtual void operator()(int32_t, const Error &) = 0;
    virtual void operator()(int32_t, const Result &) = 0;

    // update

    virtual void operator()(const AggTrade &, const server::TraceInfo &) = 0;
    virtual void operator()(const Trade &, const server::TraceInfo &) = 0;

    virtual void operator()(const MiniTicker &, const server::TraceInfo &) = 0;
    virtual void operator()(const BookTicker &, const server::TraceInfo &) = 0;

    virtual void operator()(
        const std::string_view &symbol, const Depth &depth, const server::TraceInfo &) = 0;

    virtual void operator()(
        const std::string_view &symbol,
        const DepthUpdate &depth_update,
        const server::TraceInfo &) = 0;
  };

  static void dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      const server::TraceInfo &);
};

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
