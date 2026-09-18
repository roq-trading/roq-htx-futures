/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_5_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::Positions5;

TEST_CASE("usdt_m_futures_empty", "[json_positions_5]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"positions",)"
                 R"("contract_code":"APT-USDT",)"
                 R"("ts":1789702172397,)"
                 R"("uid":"573242943",)"
                 R"("event":"snapshot",)"
                 R"("data":[])"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == protocol::json::Operator::NOTIFY); };
  Parser5Tester<value_type>::dispatch(helper, message, 8192, 2);
}

TEST_CASE("usdt_m_futures_simple", "[json_positions_5]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"positions",)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("ts":1789729970866,)"
                 R"("uid":"573242943",)"
                 R"("event":"filled",)"
                 R"("data":[{)"
                 R"("margin":"24.9963",)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("symbol":"ETH",)"
                 R"("position_mode":"dual_side",)"
                 R"("position_side":"long",)"
                 R"("direction":"buy",)"
                 R"("margin_mode":"cross",)"
                 R"("open_avg_price":"2499.82",)"
                 R"("volume":"1",)"
                 R"("available":"1",)"
                 R"("fee":"0.01499892",)"
                 R"("break_even_price":"0",)"
                 R"("total_trade_fee":"0.01499892",)"
                 R"("lever_rate":1,)"
                 R"("adl_risk_percent":null,)"
                 R"("liquidation_price":"-37042.790683015708686634",)"
                 R"("initial_margin":"24.9963",)"
                 R"("maintenance_margin":"0.08498742",)"
                 R"("profit_unreal":"-0.0019",)"
                 R"("profit":"0",)"
                 R"("profit_rate":"0",)"
                 R"("margin_rate":"0.000215613952237519",)"
                 R"("state":"NORMAL",)"
                 R"("funding_fee":"0",)"
                 R"("mark_price":"2499.63",)"
                 R"("last_price":"2499.81",)"
                 R"("contract_type":"swap",)"
                 R"("version":1,)"
                 R"("created_time":"1789729970856",)"
                 R"("updated_time":"1789729970856")"
                 R"(})"
                 R"(])"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == protocol::json::Operator::NOTIFY); };
  Parser5Tester<value_type>::dispatch(helper, message, 8192, 2);
}
