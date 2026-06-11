/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/protocol/json/auth.hpp"
#include "roq/htx_futures/protocol/json/close_2.hpp"
#include "roq/htx_futures/protocol/json/error_2.hpp"
#include "roq/htx_futures/protocol/json/ping.hpp"

#include "roq/htx_futures/protocol/json/response.hpp"

namespace roq {
namespace htx_futures {
namespace protocol {
namespace json {

struct Parser3 final {
  struct Handler {
    virtual void operator()(Trace<Close2> const &) = 0;
    virtual void operator()(Trace<Error2> const &) = 0;
    virtual void operator()(Trace<Ping> const &) = 0;
    virtual void operator()(Trace<Auth> const &) = 0;
    virtual void operator()(Trace<Response> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);
};

}  // namespace json
}  // namespace protocol
}  // namespace htx_futures
}  // namespace roq
