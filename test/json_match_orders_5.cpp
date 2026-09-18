/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_5_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::MatchOrders5;

TEST_CASE("usdt_m_futures_simple", "[json_match_orders_5]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"match_orders",)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("ts":1789709104321,)"
                 R"("uid":"573242943",)"
                 R"("data":[{)"
                 R"("side":"buy",)"
                 R"("type":"limit",)"
                 R"("price":"1000",)"
                 R"("volume":"1",)"
                 R"("state":"new",)"
                 R"("id":"100114798932887-1550497891998535681-1",)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("contract_type":"swap",)"
                 R"("order_id":"1550497891998535681",)"
                 R"("position_side":"long",)"
                 R"("price_match":null,)"
                 R"("client_order_id":"577024242451838431",)"
                 R"("margin_mode":"cross",)"
                 R"("lever_rate":"",)"
                 R"("order_source":"api",)"
                 R"("reduce_only":false,)"
                 R"("time_in_force":"gtc",)"
                 R"("cancel_reason":null,)"
                 R"("trade_id":null,)"
                 R"("trade_volume":null,)"
                 R"("total_trade_volume":"0",)"
                 R"("trade_price":null,)"
                 R"("trade_turnover":null,)"
                 R"("role":null,)"
                 R"("created_time":"1789709104310",)"
                 R"("match_time":"1789709104320",)"
                 R"("self_match_prevent":"cancel_taker",)"
                 R"("amend_origin_volume":"",)"
                 R"("amend_source":"",)"
                 R"("amend_result":"",)"
                 R"("in_time":"1789709104320",)"
                 R"("cancel_volume":"0")"
                 R"(})"
                 R"(])"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == protocol::json::Operator::NOTIFY); };
  Parser5Tester<value_type>::dispatch(helper, message, 8192, 2);
}
