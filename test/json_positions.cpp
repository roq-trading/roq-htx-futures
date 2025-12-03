/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_2_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::Positions;

TEST_CASE("coin_m_perpetual", "[json_positions]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"positions",)"
                 R"("ts":1763811216435,)"
                 R"("event":"init",)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":5,)"
                 R"("direction":"buy",)"
                 R"("last_price":84097.8,)"
                 R"("adl_risk_percent":null)"
                 R"(},{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":5,)"
                 R"("direction":"sell",)"
                 R"("last_price":84097.8,)"
                 R"("adl_risk_percent":null)"
                 R"(},{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":5,)"
                 R"("direction":"buy",)"
                 R"("last_price":2736.21,)"
                 R"("adl_risk_percent":null)"
                 R"(},{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":5,)"
                 R"("direction":"sell",)"
                 R"("last_price":2736.21,)"
                 R"("adl_risk_percent":null)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == json::Operator::NOTIFY); };
  Parser2Tester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("coin_m_perpetual_order_match", "[json_positions]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"positions.eth-usd",)"
                 R"("ts":1764049813037,)"
                 R"("event":"order.match",)"
                 R"("data":[{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":1,)"
                 R"("direction":"buy",)"
                 R"("last_price":2932.19,)"
                 R"("adl_risk_percent":4)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == json::Operator::NOTIFY); };
  Parser2Tester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("coin_m_perpetual_order_close", "[json_positions]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"positions.eth-usd",)"
                 R"("ts":1764054509308,)"
                 R"("event":"order.close",)"
                 R"("data":[{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":1.000000000000000000,)"
                 R"("available":0E-18,)"
                 R"("frozen":1.000000000000000000,)"
                 R"("cost_open":2916.960000000000153502,)"
                 R"("cost_hold":2916.960000000000153502,)"
                 R"("profit_unreal":-0.000004600890298790,)"
                 R"("profit_rate":-0.001342061296595494,)"
                 R"("profit":-0.000004600890298790,)"
                 R"("position_margin":0.003423625756621292,)"
                 R"("lever_rate":1,)"
                 R"("direction":"sell",)"
                 R"("last_price":2920.88,)"
                 R"("adl_risk_percent":3)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == json::Operator::NOTIFY); };
  Parser2Tester<value_type>::dispatch(helper, message, 8192, 1);
}
