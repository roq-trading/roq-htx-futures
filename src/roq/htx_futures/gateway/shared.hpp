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

  auto discard_symbol(std::string_view const &name) const { return dispatcher.discard_symbol(name); }

  template <typename... Args>
  auto operator()(Args &&...args) {
    return dispatcher(std::forward<Args>(args)...);
  }

 public:
  API const api;

  std::vector<MBPUpdate> bids, asks;
  std::vector<Trade> trades;

 public:
  server::Dispatcher &dispatcher;

 public:
  Settings const &settings;
  core::Symbols symbols;
  utils::unordered_set<std::string> all_symbols;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
