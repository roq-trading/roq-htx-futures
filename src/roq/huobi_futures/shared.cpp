/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/huobi_futures/shared.hpp"

#include "roq/huobi_futures/flags.hpp"

namespace roq {
namespace huobi_futures {

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : api{API::create()}, dispatcher_{dispatcher}, settings{settings},
      symbols{Flags::ws_max_subscriptions_per_stream()} {
}

}  // namespace huobi_futures
}  // namespace roq
