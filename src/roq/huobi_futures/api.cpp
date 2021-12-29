/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/api.h"

#include "roq/exceptions.h"

#include "roq/huobi_futures/flags.h"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

API API::create() {
  auto api = Flags::api();
  if (api.compare("inverse"sv) == 0) {
    return {
        .get_contract_info = "/api/v1/contract_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
    };
  }
  if (api.compare("linear"sv) == 0) {
    return {
        .get_contract_info = "/linear-swap-api/v1/swap_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
    };
  }
  if (api.compare("swap"sv) == 0) {
    return {
        .get_contract_info = "/swap-api/v1/swap_contract_info"sv,
        .market_depth = "depth.size_150.high_freq"sv,
    };
  }
  throw RuntimeErrorException(R"(Unknown api="{}")"sv, api);
}

}  // namespace huobi_futures
}  // namespace roq
