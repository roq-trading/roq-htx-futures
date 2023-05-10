/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/huobi_futures/settings.hpp"

namespace roq {
namespace huobi_futures {

struct API final {
  // rest
  std::string_view get_contract_info;
  // ws
  std::string_view market_depth;
  // index
  bool has_premium_index = {};
  bool has_estimated_rate = {};
  bool has_index = {};
  bool has_funding_rate = {};
  // factory
  static API create(Settings const &);
};

}  // namespace huobi_futures
}  // namespace roq
