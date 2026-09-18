/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_5_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::Orders5;

TEST_CASE("usdt_m_futures_simple", "[json_orders_5]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"orders",)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("ts":1789714992228,)"
                 R"("uid":"573242943",)"
                 R"("data":{)"
                 R"("side":"buy",)"
                 R"("type":"limit",)"
                 R"("price":"1000",)"
                 R"("volume":"1",)"
                 R"("state":"canceled",)"
                 R"("profit":"0",)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("position_side":"long",)"
                 R"("price_match":null,)"
                 R"("order_id":"1550512244980690944",)"
                 R"("client_order_id":"577024242486035090",)"
                 R"("margin_mode":"cross",)"
                 R"("lever_rate":1,)"
                 R"("order_source":"api",)"
                 R"("reduce_only":false,)"
                 R"("time_in_force":"gtc",)"
                 R"("trade_avg_price":"0",)"
                 R"("trade_volume":"0",)"
                 R"("trade_turnover":"0",)"
                 R"("fee_currency":null,)"
                 R"("fee":"0",)"
                 R"("tp_trigger_price":"",)"
                 R"("tp_order_price":"",)"
                 R"("tp_type":"",)"
                 R"("tp_trigger_price_type":"",)"
                 R"("sl_trigger_price":"",)"
                 R"("sl_order_price":"",)"
                 R"("sl_type":"",)"
                 R"("sl_trigger_price_type":"",)"
                 R"("contract_type":"swap",)"
                 R"("cancel_reason":"Limit order cancelation by the client",)"
                 R"("created_time":"1789712526327",)"
                 R"("updated_time":"1789714992222",)"
                 R"("self_match_prevent":"cancel_taker",)"
                 R"("amend_origin_volume":"",)"
                 R"("amend_source":"",)"
                 R"("amend_result":"",)"
                 R"("cancel_volume":"1")"
                 R"(})"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == protocol::json::Operator::NOTIFY); };
  Parser5Tester<value_type>::dispatch(helper, message, 8192, 2);
}
