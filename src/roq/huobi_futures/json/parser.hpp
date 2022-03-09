/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/buffer.hpp"

#include "roq/server.hpp"

#include "roq/huobi_futures/json/basis.hpp"
#include "roq/huobi_futures/json/bbo.hpp"
#include "roq/huobi_futures/json/depth.hpp"
#include "roq/huobi_futures/json/detail.hpp"
#include "roq/huobi_futures/json/error.hpp"
#include "roq/huobi_futures/json/estimated_rate.hpp"
#include "roq/huobi_futures/json/index.hpp"
#include "roq/huobi_futures/json/ping.hpp"
#include "roq/huobi_futures/json/premium_index.hpp"
#include "roq/huobi_futures/json/subbed.hpp"
#include "roq/huobi_futures/json/trade.hpp"

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
    virtual void operator()(const server::Trace<Index> &) = 0;
  };

  static bool dispatch(
      Handler &,
      const std::string_view &message,
      core::json::Buffer &buffer,
      const server::TraceInfo &);
};

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
