/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi_futures/shared.h"

#include "roq/huobi_futures/flags.h"

namespace roq {
namespace huobi_futures {

Shared::Shared(server::Dispatcher &dispatcher)
    : bids(server::Flags::cache_mbp_max_depth()), asks(server::Flags::cache_mbp_max_depth()),
      dispatcher_(dispatcher) {
}

}  // namespace huobi_futures
}  // namespace roq
