/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/accounts.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("simple", "[json_accounts]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"accounts",)"
                 R"("ts":1763809542628,)"
                 R"("event":"init",)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("margin_balance":0.001000000000000000,)"
                 R"("margin_static":0.00100000,)"
                 R"("margin_position":0E-18,)"
                 R"("margin_frozen":0,)"
                 R"("margin_available":0.001000000000000000,)"
                 R"("profit_real":0,)"
                 R"("profit_unreal":0E-18,)"
                 R"("withdraw_available":0.001000000000000000,)"
                 R"("risk_rate":null,)"
                 R"("liquidation_price":null,)"
                 R"("lever_rate":5,)"
                 R"("adjust_factor":0.025000000000000000)"
                 R"(},{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("margin_balance":0.010000000000000000,)"
                 R"("margin_static":0.01000000,)"
                 R"("margin_position":0E-18,)"
                 R"("margin_frozen":0,)"
                 R"("margin_available":0.010000000000000000,)"
                 R"("profit_real":0,)"
                 R"("profit_unreal":0E-18,)"
                 R"("withdraw_available":0.010000000000000000,)"
                 R"("risk_rate":null,)"
                 R"("liquidation_price":null,)"
                 R"("lever_rate":5,)"
                 R"("adjust_factor":0.025000000000000000)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  [[maybe_unused]] json::Accounts obj{message, buffers};
}
