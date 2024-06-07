/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/huobi_futures/shared.hpp"

namespace roq {
namespace huobi_futures {

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : api{API::create(settings)}, dispatcher_{dispatcher}, settings{settings}, symbols{settings.ws.max_subscriptions_per_stream} {
}

}  // namespace huobi_futures
}  // namespace roq
