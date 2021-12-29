/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string_view>

namespace roq {
namespace huobi_futures {

struct API final {
  // rest
  std::string_view get_contract_info;
  // ws
  std::string_view market_depth;
  // factory
  static API create();
};

}  // namespace huobi_futures
}  // namespace roq
