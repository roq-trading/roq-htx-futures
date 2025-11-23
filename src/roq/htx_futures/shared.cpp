/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/shared.hpp"

namespace roq {
namespace htx_futures {

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : api{API::create(settings)}, dispatcher{dispatcher}, settings{settings}, symbols{settings.ws.max_subscriptions_per_stream} {
}

}  // namespace htx_futures
}  // namespace roq
