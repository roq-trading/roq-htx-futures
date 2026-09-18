/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_5_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::Account5;

TEST_CASE("usdt_m_futures_snapshot", "[json_account_5]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"account",)"
                 R"("contract_code":"",)"
                 R"("ts":1789697876600,)"
                 R"("uid":"573242943",)"
                 R"("event":"snapshot",)"
                 R"("data":{)"
                 R"("equity":"394.181650866934552771",)"
                 R"("state":"normal",)"
                 R"("details":[{)"
                 R"("currency":"USDT",)"
                 R"("equity":"394.181650866934552771",)"
                 R"("available":"394.181650866934552771",)"
                 R"("profit_unreal":"0",)"
                 R"("initial_margin":"0",)"
                 R"("maintenance_margin":"0",)"
                 R"("maintenance_margin_rate":"0",)"
                 R"("initial_margin_rate":"0",)"
                 R"("voucher":"0",)"
                 R"("voucher_value":"0",)"
                 R"("created_time":"1789695496721",)"
                 R"("updated_time":"1789695496721",)"
                 R"("available_margin":"394.181650866934552771",)"
                 R"("isolated_equity":"0",)"
                 R"("isolated_profit_unreal":"0")"
                 R"(})"
                 R"(],)"
                 R"("initial_margin":"0",)"
                 R"("maintenance_margin":"0",)"
                 R"("maintenance_margin_rate":"0",)"
                 R"("profit_unreal":"0",)"
                 R"("available_margin":"394.181650866934552771",)"
                 R"("created_time":"1789695495933",)"
                 R"("updated_time":"1789695496721",)"
                 R"("version":3,)"
                 R"("voucher_value":"0")"
                 R"(})"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == protocol::json::Operator::NOTIFY); };
  Parser5Tester<value_type>::dispatch(helper, message, 8192, 2);
}

TEST_CASE("usdt_m_futures_create_order", "[json_account_5]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"account",)"
                 R"("contract_code":"",)"
                 R"("ts":1789716613690,)"
                 R"("uid":"573242943",)"
                 R"("event":"create_order",)"
                 R"("data":{)"
                 R"("equity":"394.181650866934552771",)"
                 R"("state":"normal",)"
                 R"("details":[{)"
                 R"("currency":"USDT",)"
                 R"("equity":"394.181650866934552771",)"
                 R"("available":"394.181650866934552771",)"
                 R"("profit_unreal":"0",)"
                 R"("initial_margin":"10.006",)"
                 R"("maintenance_margin":"0.08475554",)"
                 R"("maintenance_margin_rate":"0.000215016451967246",)"
                 R"("initial_margin_rate":"0.025384235866873886",)"
                 R"("voucher":"0",)"
                 R"("voucher_value":"0",)"
                 R"("created_time":"1789695496721",)"
                 R"("updated_time":"1789716613681",)"
                 R"("available_margin":"384.175650866934552771",)"
                 R"("isolated_equity":"0",)"
                 R"("isolated_profit_unreal":"0")"
                 R"(})"
                 R"(],)"
                 R"("initial_margin":"10.006",)"
                 R"("maintenance_margin":"0.08475554",)"
                 R"("maintenance_margin_rate":"0.000215016451967246",)"
                 R"("profit_unreal":"0",)"
                 R"("available_margin":"384.175650866934552771",)"
                 R"("created_time":"1789695495933",)"
                 R"("updated_time":"1789716613681",)"
                 R"("version":6746,)"
                 R"("voucher_value":"0")"
                 R"(})"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == protocol::json::Operator::NOTIFY); };
  Parser5Tester<value_type>::dispatch(helper, message, 8192, 2);
}
