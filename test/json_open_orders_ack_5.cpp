/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/protocol/json/open_orders_ack5.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::OpenOrdersAck5;

TEST_CASE("usdt_m_futures_empty", "[json_open_orders_ack_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":[],)"
                 R"("args":null,)"
                 R"("ts":1789704083063,)"
                 R"("unknownException":false)"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200);
    REQUIRE(std::empty(obj.data));
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("usdt_m_futures_simple", "[json_open_orders_ack_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":[{)"
                 R"("id":"1550497891998535681",)"
                 R"("side":"buy",)"
                 R"("type":"limit",)"
                 R"("price":"1000",)"
                 R"("volume":"1",)"
                 R"("state":"new",)"
                 R"("profit":null,)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("position_side":"long",)"
                 R"("price_match":null,)"
                 R"("order_id":"1550497891998535681",)"
                 R"("client_order_id":"577024242451838431",)"
                 R"("margin_mode":"cross",)"
                 R"("lever_rate":1,)"
                 R"("order_source":"api",)"
                 R"("reduce_only":false,)"
                 R"("time_in_force":"gtc",)"
                 R"("tp_trigger_price":"",)"
                 R"("tp_order_price":"",)"
                 R"("tp_type":"0",)"
                 R"("tp_trigger_price_type":"",)"
                 R"("sl_trigger_price":"",)"
                 R"("sl_order_price":"",)"
                 R"("sl_type":"0",)"
                 R"("sl_trigger_price_type":"",)"
                 R"("trade_avg_price":"0",)"
                 R"("trade_volume":"0",)"
                 R"("trade_turnover":"0",)"
                 R"("fee_currency":"",)"
                 R"("fee":"0",)"
                 R"("price_protect":false,)"
                 R"("contract_type":"swap",)"
                 R"("created_time":"1789709104310",)"
                 R"("updated_time":"1789714153083",)"
                 R"("cancel_reason":null,)"
                 R"("self_match_prevent":"cancel_taker",)"
                 R"("cancel_volume":"0")"
                 R"(},{)"
                 R"("id":"1550512244980690944",)"
                 R"("side":"buy",)"
                 R"("type":"limit",)"
                 R"("price":"1000",)"
                 R"("volume":"1",)"
                 R"("state":"new",)"
                 R"("profit":null,)"
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
                 R"("tp_trigger_price":"",)"
                 R"("tp_order_price":"",)"
                 R"("tp_type":"0",)"
                 R"("tp_trigger_price_type":"",)"
                 R"("sl_trigger_price":"",)"
                 R"("sl_order_price":"",)"
                 R"("sl_type":"0",)"
                 R"("sl_trigger_price_type":"",)"
                 R"("trade_avg_price":"0",)"
                 R"("trade_volume":"0",)"
                 R"("trade_turnover":"0",)"
                 R"("fee_currency":"",)"
                 R"("fee":"0",)"
                 R"("price_protect":false,)"
                 R"("contract_type":"swap",)"
                 R"("created_time":"1789712526327",)"
                 R"("updated_time":"1789714153083",)"
                 R"("cancel_reason":null,)"
                 R"("self_match_prevent":"cancel_taker",)"
                 R"("cancel_volume":"0")"
                 R"(})"
                 R"(],)"
                 R"("args":null,)"
                 R"("ts":1789714153658,)"
                 R"("unknownException":false)"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200);
    REQUIRE(std::size(obj.data) == 2);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}
