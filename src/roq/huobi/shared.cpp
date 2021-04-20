/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi/shared.h"

#include <magic_enum.hpp>

#include "roq/huobi/flags.h"

namespace roq {
namespace huobi {

Shared::Shared(server::Dispatcher &dispatcher)
    : bids(server::Flags::cache_mbp_max_depth()), asks(server::Flags::cache_mbp_max_depth()),
      dispatcher_(dispatcher) {
}

}  // namespace huobi
}  // namespace roq
