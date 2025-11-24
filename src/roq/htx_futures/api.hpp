/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/htx_futures/settings.hpp"

namespace roq {
namespace htx_futures {

struct API final {
  struct {
    std::string_view get_contract_info;
    std::string_view market_depth;
    bool has_premium_index = {};
    bool has_estimated_rate = {};
    bool has_index = {};
    bool has_funding_rate = {};
  } market_data;
  struct {
    std::string_view account_info = {};
    std::string_view open_orders = {};
    std::string_view place_order = {};
    std::string_view cancel_order = {};
    std::string_view cancel_all_orders = {};
  } order_management;

  // factory
  static API create(Settings const &);
};

}  // namespace htx_futures
}  // namespace roq
