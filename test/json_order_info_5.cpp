/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/protocol/json/order_info_ack5.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::OrderInfoAck5;

TEST_CASE("usdt_m_futures_does_not_exist", "[json_order_info_ack_5]") {
  auto message = R"({)"
                 R"("code":1061,)"
                 R"("message":"The order does not exist.",)"
                 R"("data":null,)"
                 R"("ts":1793504784995)"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 1061);
    CHECK(obj.message == "The order does not exist."sv);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("usdt_m_futures_new", "[json_order_info_ack_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":{)"
                 R"("id":"1566430900657094661",)"
                 R"("side":"buy",)"
                 R"("type":"limit",)"
                 R"("price":"1500",)"
                 R"("volume":"1",)"
                 R"("state":"new",)"
                 R"("profit":null,)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("position_side":"long",)"
                 R"("price_match":null,)"
                 R"("order_id":"1566430900657094661",)"
                 R"("client_order_id":"577024280439101322",)"
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
                 R"("real_profit":null,)"
                 R"("contract_type":"swap",)"
                 R"("created_time":"1793507829595",)"
                 R"("updated_time":"1793507829595",)"
                 R"("cancel_reason":null,)"
                 R"("self_match_prevent":"cancel_taker",)"
                 R"("cancel_volume":"0")"
                 R"(},)"
                 R"("ts":1793507829865)"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200);
    CHECK(obj.message == "Success"sv);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("usdt_m_futures_canceled", "[json_order_info_ack_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":{)"
                 R"("id":null,)"
                 R"("side":"buy",)"
                 R"("type":"limit",)"
                 R"("price":"1500",)"
                 R"("volume":"1",)"
                 R"("state":"canceled",)"
                 R"("profit":"0",)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("position_side":"long",)"
                 R"("price_match":null,)"
                 R"("order_id":"1566430900657094661",)"
                 R"("client_order_id":"577024280439101322",)"
                 R"("margin_mode":"cross",)"
                 R"("lever_rate":1,)"
                 R"("order_source":"api",)"
                 R"("reduce_only":false,)"
                 R"("time_in_force":"gtc",)"
                 R"("tp_trigger_price":null,)"
                 R"("tp_order_price":null,)"
                 R"("tp_type":null,)"
                 R"("tp_trigger_price_type":null,)"
                 R"("sl_trigger_price":null,)"
                 R"("sl_order_price":null,)"
                 R"("sl_type":null,)"
                 R"("sl_trigger_price_type":null,)"
                 R"("trade_avg_price":"0",)"
                 R"("trade_volume":"0",)"
                 R"("trade_turnover":"0",)"
                 R"("fee_currency":"USDT",)"
                 R"("fee":"0",)"
                 R"("price_protect":false,)"
                 R"("real_profit":"0",)"
                 R"("contract_type":"swap",)"
                 R"("created_time":"1793507829595",)"
                 R"("updated_time":"1793507831786",)"
                 R"("cancel_reason":"Limit order cancelation by the client",)"
                 R"("self_match_prevent":"cancel_taker",)"
                 R"("cancel_volume":"1")"
                 R"(},)"
                 R"("ts":1793507832045)"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200);
    CHECK(obj.message == "Success"sv);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}
