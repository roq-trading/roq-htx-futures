/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/htx_futures/settings.hpp"

namespace roq {
namespace htx_futures {

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
  // order management
  std::string_view cancel_all_orders = {};
  // factory
  static API create(Settings const &);
};

}  // namespace htx_futures
}  // namespace roq
