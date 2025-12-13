/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/basis.hpp"
#include "roq/htx_futures/json/bbo.hpp"
#include "roq/htx_futures/json/depth.hpp"
#include "roq/htx_futures/json/detail.hpp"
#include "roq/htx_futures/json/error.hpp"
#include "roq/htx_futures/json/estimated_rate.hpp"
#include "roq/htx_futures/json/index.hpp"
#include "roq/htx_futures/json/ping.hpp"
#include "roq/htx_futures/json/premium_index.hpp"
#include "roq/htx_futures/json/subbed.hpp"
#include "roq/htx_futures/json/trade.hpp"

namespace roq {
namespace htx_futures {
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

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);
};

}  // namespace json
}  // namespace htx_futures
}  // namespace roq
