/* Copyright (c) 2017-2025, Hans Erik Thrane */

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
        .has_premium_index = false,
        .has_estimated_rate = false,
        .has_index = false,
        .has_funding_rate = false,
    },
    .order_management{
        .account_info = "/linear-swap-api/v1/swap_account_info"sv,
        .place_order = "/linear-swap-api/v1/swap_order"sv,
        .cancel_order = "/linear-swap-api/v1/swap_cancel"sv,
        .cancel_all_orders = "/linear-swap-api/v1/swap_cancelall"sv,
        // .account_info = "/linear-swap-api/v1/swap_cross_account_info"sv,
        // .place_order = "/linear-swap-api/v1/swap_cross_order"sv,
        // .cancel_order = "/linear-swap-api/v1/swap_cross_cancel"sv,
        // .cancel_all_orders = "/linear-swap-api/v1/swap_cross_cancelall"sv,
    },
};

auto const API_COIN_M_DELIVERY = API{
    .market_data{
        .get_contract_info = "/api/v1/contract_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
        .has_premium_index = true,
        .has_estimated_rate = true,
        .has_index = true,
        .has_funding_rate = true,
    },
    .order_management{
        .account_info = "/api/v1/contract_account_info"sv,
        .place_order = "/api/v1/contract_order"sv,
        .cancel_order = "/api/v1/contract_cancel"sv,
        .cancel_all_orders = "/api/v1/contract_cancelall"sv,
    },
};

auto const API_COIN_M_PERPETUAL = API{
    .market_data{
        .get_contract_info = "/swap-api/v1/swap_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
        .has_premium_index = true,
        .has_estimated_rate = true,
        .has_index = true,
        .has_funding_rate = true,
    },
    .order_management{
        .account_info = "/swap-api/v1/swap_account_info"sv,
        .place_order = "/swap-api/v1/swap_order"sv,
        .cancel_order = "/swap-api/v1/swap_cancel"sv,
        .cancel_all_orders = "/swap-api/v1/swap_cancelall"sv,
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
