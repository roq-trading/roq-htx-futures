/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <utility>

#include "roq/api.h"
#include "roq/server.h"

#include "roq/core/memory.h"

namespace roq {
namespace huobi_futures {

struct Shared final {
  explicit Shared(server::Dispatcher &);

  Shared(Shared &&) = default;
  Shared(const Shared &) = delete;

  auto discard_symbol(const std::string_view &name) const {
    return dispatcher_.discard_symbol(name);
  }

  template <typename... Args>
  auto update_order(Args &&...args) {
    return dispatcher_.update_order(std::forward<Args>(args)...);
  }

 public:
  core::page_aligned_vector<MBPUpdate> bids, asks;

 private:
  server::Dispatcher &dispatcher_;
};

}  // namespace huobi_futures
}  // namespace roq
