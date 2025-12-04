/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/open_orders_ack.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::OpenOrdersAck;

TEST_CASE("coin_m_perpetual_empty", "[json_open_orders_ack]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":{)"
                 R"("orders":[],)"
                 R"("total_page":1,)"
                 R"("current_page":1,)"
                 R"("total_size":0)"
                 R"(},)"
                 R"("ts":1763987974231)"
                 R"(})";
  auto helper = [&](value_type &obj) { CHECK(obj.status == json::Status::OK); };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("coin_m_perpetual_1", "[json_open_orders_ack]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":{)"
                 R"("orders":[{)"
                 R"("update_time":1763988784302,)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("volume":1,)"
                 R"("price":200000.0,)"
                 R"("order_price_type":"limit",)"
                 R"("order_type":1,)"
                 R"("direction":"sell",)"
                 R"("offset":"open",)"
                 R"("lever_rate":1,)"
                 R"("order_id":1442619050865754112,)"
                 R"("client_order_id":563232945014195,)"
                 R"("created_at":1763988784286,)"
                 R"("trade_volume":0,)"
                 R"("trade_turnover":0,)"
                 R"("fee":0,)"
                 R"("trade_avg_price":null,)"
                 R"("margin_frozen":0.000500000000000000,)"
                 R"("profit":0,)"
                 R"("status":3,)"
                 R"("order_source":"api",)"
                 R"("canceled_source":null,)"
                 R"("order_id_str":"1442619050865754112",)"
                 R"("fee_asset":"BTC",)"
                 R"("liquidation_type":null,)"
                 R"("canceled_at":null,)"
                 R"("is_tpsl":0,)"
                 R"("real_profit":0,)"
                 R"("self_match_prevent":1)"
                 R"(})"
                 R"(],)"
                 R"("total_page":1,)"
                 R"("current_page":1,)"
                 R"("total_size":1},)"
                 R"("ts":1763988814735)"
                 R"(})";
  auto helper = [&](value_type &obj) { CHECK(obj.status == json::Status::OK); };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("coin_m_perpetual_2", "[json_open_orders_ack]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":{)"
                 R"("orders":[{)"
                 R"("update_time":1764651043496,)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":1,)"
                 R"("price":2000.00,)"
                 R"("order_price_type":"limit",)"
                 R"("order_type":1,)"
                 R"("direction":"buy",)"
                 R"("offset":"open",)"
                 R"("lever_rate":1,)"
                 R"("order_id":1445396767259586560,)"
                 R"("client_order_id":563239567803652,)"
                 R"("created_at":1764651043478,)"
                 R"("trade_volume":0,)"
                 R"("trade_turnover":0,)"
                 R"("fee":0,)"
                 R"("trade_avg_price":null,)"
                 R"("margin_frozen":0.005000000000000000,)"
                 R"("profit":0,)"
                 R"("status":3,)"
                 R"("order_source":"api",)"
                 R"("canceled_source":null,)"
                 R"("order_id_str":"1445396767259586560",)"
                 R"("fee_asset":"ETH",)"
                 R"("liquidation_type":null,)"
                 R"("canceled_at":null,)"
                 R"("is_tpsl":0,)"
                 R"("real_profit":0,)"
                 R"("self_match_prevent":1,)"
                 R"("tp_trigger_price":null,)"
                 R"("sl_trigger_price":null)"
                 R"(})"
                 R"(],)"
                 R"("total_page":1,)"
                 R"("current_page":1,)"
                 R"("total_size":1},)"
                 R"("ts":1764653131151)"
                 R"(})";
  auto helper = [&](value_type &obj) { CHECK(obj.status == json::Status::OK); };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("usdt_m_futures", "[json_open_orders_ack]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":{)"
                 R"("orders":[{)"
                 R"("update_time":1764843687518,)"
                 R"("updateTime":1764843687518,)"
                 R"("business_type":"swap",)"
                 R"("contract_type":"swap",)"
                 R"("pair":"APT-USDT",)"
                 R"("remark2":"0-null-1764843687518-0",)"
                 R"("symbol":"APT",)"
                 R"("contract_code":"APT-USDT",)"
                 R"("volume":1,)"
                 R"("price":1.0000,)"
                 R"("order_price_type":"limit",)"
                 R"("order_type":1,)"
                 R"("direction":"buy",)"
                 R"("offset":"open",)"
                 R"("lever_rate":1,)"
                 R"("order_id":1446204774843723776,)"
                 R"("client_order_id":563241493928254,)"
                 R"("created_at":1764843687501,)"
                 R"("trade_volume":0,)"
                 R"("trade_turnover":0,)"
                 R"("fee":0,)"
                 R"("trade_avg_price":null,)"
                 R"("margin_frozen":0.100000000000000000,)"
                 R"("profit":0,)"
                 R"("status":3,)"
                 R"("order_source":"api",)"
                 R"("canceled_source":null,)"
                 R"("order_id_str":"1446204774843723776",)"
                 R"("fee_asset":"USDT",)"
                 R"("fee_amount":0,)"
                 R"("fee_quote_amount":0,)"
                 R"("liquidation_type":"0",)"
                 R"("canceled_at":0,)"
                 R"("margin_asset":"USDT",)"
                 R"("margin_account":"USDT",)"
                 R"("margin_mode":"cross",)"
                 R"("is_tpsl":0,)"
                 R"("real_profit":0,)"
                 R"("trade_partition":"USDT",)"
                 R"("reduce_only":0,)"
                 R"("self_match_prevent":1,)"
                 R"("tp_trigger_price":null,)"
                 R"("sl_trigger_price":null,)"
                 R"("self_match_prevent_new":"cancel_taker")"
                 R"(})"
                 R"(],)"
                 R"("total_page":1,)"
                 R"("current_page":1,)"
                 R"("total_size":1,)"
                 R"("exceed_limit":false)"
                 R"(},)"
                 R"("ts":1764843734292)"
                 R"(})";
  auto helper = [&](value_type &obj) { CHECK(obj.status == json::Status::OK); };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}
