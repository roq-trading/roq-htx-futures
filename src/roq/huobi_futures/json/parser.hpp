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
    virtual void operator()(Trace<Ping> const &) = 0;
    virtual void operator()(Trace<Error> const &) = 0;
    virtual void operator()(Trace<Subbed> const &) = 0;
    virtual void operator()(Trace<BBO> const &) = 0;
    virtual void operator()(Trace<Depth> const &) = 0;
    virtual void operator()(Trace<Trade> const &) = 0;
    virtual void operator()(Trace<Detail> const &) = 0;
    virtual void operator()(Trace<EstimatedRate> const &) = 0;
    virtual void operator()(Trace<PremiumIndex> const &) = 0;
    virtual void operator()(Trace<Basis> const &) = 0;
    virtual void operator()(Trace<Index> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::Buffer &, TraceInfo const &);
};

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
