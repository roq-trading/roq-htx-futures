/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/gateway/request.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace gateway {

// === IMPLEMENTATION ===

Request::Request(Shared &shared) : orders{*this}, shared_{shared} {
}

bool Request::ready() const {
  return orders.ready();
}

bool Request::can_request(TraceInfo const &trace_info) {
  return shared_.rate_limiter.can_request(trace_info);
}

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
