/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/auth.hpp"
#include "roq/htx_futures/json/close.hpp"
#include "roq/htx_futures/json/error_2.hpp"
#include "roq/htx_futures/json/ping.hpp"
#include "roq/htx_futures/json/sub.hpp"

#include "roq/htx_futures/json/funding_rate.hpp"

#include "roq/htx_futures/json/accounts.hpp"
#include "roq/htx_futures/json/match_orders.hpp"
#include "roq/htx_futures/json/orders.hpp"
#include "roq/htx_futures/json/positions.hpp"

namespace roq {
namespace htx_futures {
namespace json {

struct Parser2 final {
  struct Handler {
    virtual void operator()(Trace<Close> const &) = 0;
    virtual void operator()(Trace<Error2> const &) = 0;
    virtual void operator()(Trace<Ping> const &) = 0;
    virtual void operator()(Trace<Auth> const &) = 0;
    virtual void operator()(Trace<Sub> const &) = 0;
    virtual void operator()(Trace<FundingRate> const &) = 0;
    virtual void operator()(Trace<Accounts> const &) = 0;
    virtual void operator()(Trace<Positions> const &) = 0;
    virtual void operator()(Trace<MatchOrders> const &) = 0;
    virtual void operator()(Trace<Orders> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);
};

}  // namespace json
}  // namespace htx_futures
}  // namespace roq
