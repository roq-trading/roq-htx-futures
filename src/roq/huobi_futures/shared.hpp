/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <utility>
#include <vector>

#include "roq/api.hpp"
#include "roq/server.hpp"

#include "roq/core/symbols.hpp"

#include "roq/huobi_futures/api.hpp"
#include "roq/huobi_futures/settings.hpp"

namespace roq {
namespace huobi_futures {

struct Shared final {
  Shared(server::Dispatcher &, Settings const &);

  Shared(Shared const &) = delete;

  auto discard_symbol(std::string_view const &name) const { return dispatcher_.discard_symbol(name); }

  template <typename... Args>
  auto update_order(Args &&...args) {
    return dispatcher_.update_order(std::forward<Args>(args)...);
  }

 public:
  API const api;

  std::vector<MBPUpdate> bids, asks;
  std::vector<Trade> trades;

 private:
  server::Dispatcher &dispatcher_;

 public:
  Settings const &settings;
  core::Symbols symbols;
};

}  // namespace huobi_futures
}  // namespace roq
