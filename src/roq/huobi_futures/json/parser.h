/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/buffer.h"

#include "roq/server.h"

#include "roq/huobi_futures/json/basis.h"
#include "roq/huobi_futures/json/bbo.h"
#include "roq/huobi_futures/json/depth.h"
#include "roq/huobi_futures/json/detail.h"
#include "roq/huobi_futures/json/error.h"
#include "roq/huobi_futures/json/estimated_rate.h"
#include "roq/huobi_futures/json/ping.h"
#include "roq/huobi_futures/json/premium_index.h"
#include "roq/huobi_futures/json/subbed.h"
#include "roq/huobi_futures/json/trade.h"

namespace roq {
namespace huobi_futures {
namespace json {

struct Parser final {
  struct Handler {
    virtual void operator()(const server::Trace<Ping> &) = 0;
    virtual void operator()(const server::Trace<Error> &) = 0;
    virtual void operator()(const server::Trace<Subbed> &) = 0;
    virtual void operator()(const server::Trace<BBO> &) = 0;
    virtual void operator()(const server::Trace<Depth> &) = 0;
    virtual void operator()(const server::Trace<Trade> &) = 0;
    virtual void operator()(const server::Trace<Detail> &) = 0;
    virtual void operator()(const server::Trace<EstimatedRate> &) = 0;
    virtual void operator()(const server::Trace<PremiumIndex> &) = 0;
    virtual void operator()(const server::Trace<Basis> &) = 0;
  };

  static bool dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      const server::TraceInfo &);
};

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
