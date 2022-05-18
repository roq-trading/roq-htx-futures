/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <utility>

#include "roq/api.hpp"
#include "roq/server.hpp"

#include "roq/core/memory.hpp"
#include "roq/core/symbols.hpp"

#include "roq/huobi_futures/api.hpp"

namespace roq {
namespace huobi_futures {

struct Shared final {
  explicit Shared(server::Dispatcher &);

  Shared(Shared &&) = default;
  Shared(Shared const &) = delete;

  auto discard_symbol(std::string_view const &name) const { return dispatcher_.discard_symbol(name); }

  template <typename... Args>
  auto update_order(Args &&...args) {
    return dispatcher_.update_order(std::forward<Args>(args)...);
  }

 public:
  const API api;
  core::page_aligned_vector<MBPUpdate> bids, asks, final_bids, final_asks;
  core::page_aligned_vector<Trade> trades;

 private:
  server::Dispatcher &dispatcher_;

 public:
  core::Symbols symbols;
};

}  // namespace huobi_futures
}  // namespace roq
