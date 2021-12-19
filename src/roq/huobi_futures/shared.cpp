/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/shared.h"

#include "roq/huobi_futures/flags.h"

namespace roq {
namespace huobi_futures {

Shared::Shared(server::Dispatcher &dispatcher)
    : bids(server::Flags::cache_mbp_max_depth()), asks(server::Flags::cache_mbp_max_depth()),
      final_bids(server::Flags::cache_mbp_max_depth()),
      final_asks(server::Flags::cache_mbp_max_depth()),
      trades(server::Flags::cache_trades_max_depth()), dispatcher_(dispatcher),
      symbols(Flags::ws_max_subscriptions_per_stream()) {
}

}  // namespace huobi_futures
}  // namespace roq
