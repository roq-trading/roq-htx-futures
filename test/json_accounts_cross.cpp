/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_2_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::AccountsCross;

TEST_CASE("usdt_m_futures_init", "[json_accounts_across]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"accounts_cross",)"
                 R"("ts":1764844893794,)"
                 R"("event":"init",)"
                 R"("data":[{)"
                 R"("margin_mode":"cross",)"
                 R"("margin_account":"USDT",)"
                 R"("margin_asset":"USDT",)"
                 R"("margin_balance":100.535351780000000000,)"
                 R"("margin_static":100.535351780000000000,)"
                 R"("margin_position":0,)"
                 R"("margin_frozen":0E-18,)"
                 R"("profit_real":-0.253675300000000000,)"
                 R"("profit_unreal":0E-18,)"
                 R"("withdraw_available":100.535351780000000000,)"
                 R"("risk_rate":null,)"
                 R"("position_mode":"dual_side",)"
                 R"("contract_detail":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USDT",)"
                 R"("margin_position":0,)"
                 R"("margin_frozen":0E-18,)"
                 R"("margin_available":100.535351780000000000,)"
                 R"("profit_unreal":0E-18,)"
                 R"("liquidation_price":null,)"
                 R"("lever_rate":10,)"
                 R"("adjust_factor":0.040000000000000000,)"
                 R"("contract_type":"swap",)"
                 R"("pair":"BTC-USDT",)"
                 R"("business_type":"swap",)"
                 R"("trade_partition":"USDT")"
                 R"(},{)"
                 R"("symbol":"APT",)"
                 R"("contract_code":"APT-USDT",)"
                 R"("margin_position":0,)"
                 R"("margin_frozen":0E-18,)"
                 R"("margin_available":100.535351780000000000,)"
                 R"("profit_unreal":0E-18,)"
                 R"("liquidation_price":null,)"
                 R"("lever_rate":1,)"
                 R"("adjust_factor":0.020000000000000000,)"
                 R"("contract_type":"swap",)"
                 R"("pair":"APT-USDT",)"
                 R"("business_type":"swap",)"
                 R"("trade_partition":"USDT")"
                 R"(})"
                 R"(],)"
                 R"("futures_contract_detail":[])"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == json::Operator::NOTIFY); };
  Parser2Tester<value_type>::dispatch(helper, message, 8192, 2);
}
