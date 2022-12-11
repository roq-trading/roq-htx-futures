/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/huobi_futures/api.hpp"

#include "roq/exceptions.hpp"

#include "roq/huobi_futures/flags.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

// === IMPLEMENTATION ===

API API::create() {
  auto api = Flags::api();
  if (std::empty(api)) {
    return {
        .get_contract_info = "/api/v1/contract_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
        .has_premium_index = false,
        .has_estimated_rate = false,
        .has_index = false,
        .has_funding_rate = false,
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
    };
  }
  throw RuntimeError{R"(Unknown api="{}")"sv, api};
}

}  // namespace huobi_futures
}  // namespace roq
