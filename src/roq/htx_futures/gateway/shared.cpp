/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/gateway/shared.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : api{API::create(settings)}, dispatcher{dispatcher}, settings{settings}, symbols{settings.ws.max_subscriptions_per_stream} {
}

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
