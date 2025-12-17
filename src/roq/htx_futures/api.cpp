/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/api.hpp"

#include "roq/logging.hpp"

#include "roq/utils/enum.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {

// === CONSTANTS ===

namespace {
auto const API_USDT_M_FUTURES = API{
    .market_data{
        .get_contract_info = "/linear-swap-api/v1/swap_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
    },
    .order_management{
        .default_margin_mode = MarginMode::CROSS,
        // isolated
        .account_info = "/linear-swap-api/v1/swap_account_info"sv,
        .open_orders = "/linear-swap-api/v1/swap_openorders"sv,
        .place_order = "/linear-swap-api/v1/swap_order"sv,
        .cancel_order = "/linear-swap-api/v1/swap_cancel"sv,
        .cancel_all_orders = "/linear-swap-api/v1/swap_cancelall"sv,
        .topic_accounts = "accounts.*"sv,
        .topic_positions = "positions.*"sv,
        .topic_match_orders = "matchOrders.*"sv,
        .topic_orders = "orders.*"sv,
        // cross
        .account_info_cross = "/linear-swap-api/v1/swap_cross_account_info"sv,
        .open_orders_cross = "/linear-swap-api/v1/swap_cross_openorders"sv,
        .place_order_cross = "/linear-swap-api/v1/swap_cross_order"sv,
        .cancel_order_cross = "/linear-swap-api/v1/swap_cross_cancel"sv,
        .cancel_all_orders_cross = "/linear-swap-api/v1/swap_cross_cancelall"sv,
        .topic_accounts_cross = "accounts_cross.*"sv,
        .topic_positions_cross = "positions_cross.*"sv,
        .topic_match_orders_cross = "matchOrders_cross.*"sv,
        .topic_orders_cross = "orders_cross.*"sv,
    },
};

auto const API_COIN_M_DELIVERY = API{
    .market_data{
        .get_contract_info = "/api/v1/contract_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
    },
    .order_management{
        .default_margin_mode = MarginMode::ISOLATED,
        // isolated
        .account_info = "/api/v1/contract_account_info"sv,
        .open_orders = "/api/v1/contract_openorders"sv,
        .place_order = "/api/v1/contract_order"sv,
        .cancel_order = "/api/v1/contract_cancel"sv,
        .cancel_all_orders = "/api/v1/contract_cancelall"sv,
        .topic_accounts = "accounts.*"sv,
        .topic_positions = "positions.*"sv,
        .topic_match_orders = "matchOrders.*"sv,
        .topic_orders = "orders.*"sv,
        // cross (duplicate)
        .account_info_cross = "/api/v1/contract_account_info"sv,
        .open_orders_cross = "/api/v1/contract_openorders"sv,
        .place_order_cross = "/api/v1/contract_order"sv,
        .cancel_order_cross = "/api/v1/contract_cancel"sv,
        .cancel_all_orders_cross = "/api/v1/contract_cancelall"sv,
        .topic_accounts_cross = "accounts.*"sv,
        .topic_positions_cross = "positions.*"sv,
        .topic_match_orders_cross = "matchOrders.*"sv,
        .topic_orders_cross = "orders.*"sv,
    },
};

auto const API_COIN_M_PERPETUAL = API{
    .market_data{
        .get_contract_info = "/swap-api/v1/swap_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
    },
    .order_management{
        .default_margin_mode = MarginMode::ISOLATED,
        // isolated
        .account_info = "/swap-api/v1/swap_account_info"sv,
        .open_orders = "/swap-api/v1/swap_openorders"sv,
        .place_order = "/swap-api/v1/swap_order"sv,
        .cancel_order = "/swap-api/v1/swap_cancel"sv,
        .cancel_all_orders = "/swap-api/v1/swap_cancelall"sv,
        .topic_accounts = "accounts.*"sv,
        .topic_positions = "positions.*"sv,
        .topic_match_orders = "matchOrders.*"sv,
        .topic_orders = "orders.*"sv,
        // cross (duplicate)
        .account_info_cross = "/swap-api/v1/swap_account_info"sv,
        .open_orders_cross = "/swap-api/v1/swap_openorders"sv,
        .place_order_cross = "/swap-api/v1/swap_order"sv,
        .cancel_order_cross = "/swap-api/v1/swap_cancel"sv,
        .cancel_all_orders_cross = "/swap-api/v1/swap_cancelall"sv,
        .topic_accounts_cross = "accounts.*"sv,
        .topic_positions_cross = "positions.*"sv,
        .topic_match_orders_cross = "matchOrders.*"sv,
        .topic_orders_cross = "orders.*"sv,
    },
};
}  // namespace

// === HELPERS ===

namespace {
enum class Type {
  USDT_M_FUTURES,
  COIN_M_DELIVERY,
  COIN_M_PERPETUAL,
};

auto parse_api(auto &api) {
  std::string tmp{api};
  std::replace(tmp.begin(), tmp.end(), '-', '_');
  return utils::parse_enum<Type>(tmp);
}
}  // namespace

// === IMPLEMENTATION ===

API API::create(Settings const &settings) {
  auto key = parse_api(settings.app.api);
  switch (key) {
    using enum Type;
    case USDT_M_FUTURES:
      return API_USDT_M_FUTURES;
    case COIN_M_DELIVERY:
      return API_COIN_M_DELIVERY;
    case COIN_M_PERPETUAL:
      return API_COIN_M_PERPETUAL;
  }
  log::fatal("Unexpected"sv);
}

}  // namespace htx_futures
}  // namespace roq
