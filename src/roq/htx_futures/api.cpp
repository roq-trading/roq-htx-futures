/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/api.hpp"

#include "roq/exceptions.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {

// === IMPLEMENTATION ===

API API::create(Settings const &settings) {
  auto api = settings.app.api;
  if (std::empty(api)) {
    return {
        .get_contract_info = "/api/v1/contract_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
        .has_premium_index = false,
        .has_estimated_rate = false,
        .has_index = false,
        .has_funding_rate = false,
        .cancel_all_orders = ""sv,
    };
  }
  if (api.compare("linear-swap"sv) == 0) {
    return {
        .get_contract_info = "/linear-swap-api/v1/swap_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
        .has_premium_index = true,
        .has_estimated_rate = true,
        .has_index = true,
        .has_funding_rate = true,
        .cancel_all_orders = "/linear-swap-api/v1/swap_cancelall"sv,
    };
  }
  if (api.compare("swap"sv) == 0) {
    return {
        .get_contract_info = "/swap-api/v1/swap_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
        .has_premium_index = true,
        .has_estimated_rate = true,
        .has_index = true,
        .has_funding_rate = true,
        .cancel_all_orders = "/swap-api/v1/swap_cancelall"sv,
    };
  }
  throw RuntimeError{R"(Unknown api="{}")"sv, api};
}

}  // namespace htx_futures
}  // namespace roq
