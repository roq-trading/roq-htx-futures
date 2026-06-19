/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <utility>
#include <vector>

#include "roq/api.hpp"
#include "roq/server.hpp"

#include "roq/core/symbols.hpp"

#include "roq/htx_futures/gateway/api.hpp"
#include "roq/htx_futures/gateway/settings.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct Shared final {
  Shared(server::Dispatcher &, Settings const &);

  Shared(Shared const &) = delete;

  server::Dispatcher &dispatcher;

  Settings const &settings;
  API const api;

  core::Symbols symbols;
  utils::unordered_set<std::string> all_symbols;

  std::vector<MBPUpdate> bids, asks, final_bids, final_asks;
  std::vector<Trade> trades;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
