/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/htx_futures/settings.hpp"

namespace roq {
namespace htx_futures {

struct API final {
  struct {
    std::string_view get_contract_info;
    std::string_view market_depth;
  } market_data;
  struct {
    MarginMode default_margin_mode = {};
    // isolated
    std::string_view account_info = {};
    std::string_view open_orders = {};
    std::string_view place_order = {};
    std::string_view cancel_order = {};
    std::string_view cancel_all_orders = {};
    std::string_view topic_accounts = {};
    std::string_view topic_positions = {};
    std::string_view topic_match_orders = {};
    std::string_view topic_orders = {};
    // cross
    std::string_view account_info_cross = {};
    std::string_view open_orders_cross = {};
    std::string_view place_order_cross = {};
    std::string_view cancel_order_cross = {};
    std::string_view cancel_all_orders_cross = {};
    std::string_view topic_accounts_cross = {};
    std::string_view topic_positions_cross = {};
    std::string_view topic_match_orders_cross = {};
    std::string_view topic_orders_cross = {};
  } order_management;

  // factory
  static API create(Settings const &);

  enum class Key {
    USDT_M_FUTURES,
    COIN_M_DELIVERY,
    COIN_M_PERPETUAL,
  };

  static Key parse_api(Settings const &);
};

}  // namespace htx_futures
}  // namespace roq
