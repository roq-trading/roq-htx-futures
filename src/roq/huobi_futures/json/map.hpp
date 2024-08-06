/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <tuple>

#include "roq/api.hpp"

#include "roq/huobi_futures/json/side.hpp"

namespace roq {
namespace huobi_futures {
namespace json {

template <typename... Args>
struct Map final {
  explicit Map(Args &&...args) : args_{std::forward<Args>(args)...} {}
  explicit Map(Args const &...args) : args_{args...} {}

  Map(Map const &) = delete;

  template <typename R>
  operator R();

 private:
  std::tuple<Args...> const args_;
};

template <typename R, typename... Args>
inline R map(Args &&...args) {
  return static_cast<R>(Map{std::forward<Args>(args)...});
}

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
