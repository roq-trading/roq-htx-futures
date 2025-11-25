/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/orders.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("coin_m_perpetual_place", "[json_orders]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"orders.btc-usd",)"
                 R"("ts":1763959722536,)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("volume":1,)"
                 R"("price":200000.0,)"
                 R"("order_price_type":"limit",)"
                 R"("direction":"sell",)"
                 R"("offset":"open",)"
                 R"("status":3,)"
                 R"("lever_rate":1,)"
                 R"("order_id":1442497156971790336,)"
                 R"("order_id_str":"1442497156971790336",)"
                 R"("client_order_id":563232654517777,)"
                 R"("order_source":"api",)"
                 R"("order_type":1,)"
                 R"("created_at":1763959722516,)"
                 R"("trade_volume":0,)"
                 R"("trade_turnover":0,)"
                 R"("fee":0,)"
                 R"("trade_avg_price":0,)"
                 R"("margin_frozen":0.000500000000000000,)"
                 R"("profit":0,)"
                 R"("trade":[],)"
                 R"("canceled_at":0,)"
                 R"("fee_asset":"BTC",)"
                 R"("uid":"573242943",)"
                 R"("liquidation_type":"0",)"
                 R"("is_tpsl":0,)"
                 R"("real_profit":0,)"
                 R"("self-match-prevent":1,)"
                 R"("canceled_source":null)"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  [[maybe_unused]] json::Orders obj{message, buffers};
}

TEST_CASE("coin_m_perpetual_cancel", "[json_orders]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"orders.btc-usd",)"
                 R"("ts":1763958470474,)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("volume":1,)"
                 R"("price":200000.0,)"
                 R"("order_price_type":"limit",)"
                 R"("direction":"sell",)"
                 R"("offset":"open",)"
                 R"("status":7,)"
                 R"("lever_rate":1,)"
                 R"("order_id":1442490713924456448,)"
                 R"("order_id_str":"1442490713924456448",)"
                 R"("client_order_id":563232638917915,)"
                 R"("order_source":"api",)"
                 R"("order_type":2,)"
                 R"("created_at":1763958186374,)"
                 R"("trade_volume":0,)"
                 R"("trade_turnover":0,)"
                 R"("fee":0,)"
                 R"("trade_avg_price":0,)"
                 R"("margin_frozen":0E-18,)"
                 R"("profit":0,)"
                 R"("trade":[],)"
                 R"("canceled_at":1763958470464,)"
                 R"("fee_asset":"BTC",)"
                 R"("uid":"573242943",)"
                 R"("liquidation_type":"0",)"
                 R"("is_tpsl":0,)"
                 R"("real_profit":0,)"
                 R"("self-match-prevent":1,)"
                 R"("canceled_source":"api")"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  [[maybe_unused]] json::Orders obj{message, buffers};
}

TEST_CASE("coin_m_perpetual_fill", "[json_orders]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"orders.eth-usd",)"
                 R"("ts":1764048458804,)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":1,)"
                 R"("price":2900.00,)"
                 R"("order_price_type":"limit",)"
                 R"("direction":"sell",)"
                 R"("offset":"open",)"
                 R"("status":6,)"
                 R"("lever_rate":1,)"
                 R"("order_id":1442869343784435712,)"
                 R"("order_id_str":"1442869343784435712",)"
                 R"("client_order_id":563233541675078,)"
                 R"("order_source":"api",)"
                 R"("order_type":1,)"
                 R"("created_at":1764048458767,)"
                 R"("trade_volume":1,)"
                 R"("trade_turnover":10.000000000000000000,)"
                 R"("fee":-0.000001714113323460,)"
                 R"("trade_avg_price":2916.960000000000153500000000000000000000,)"
                 R"("margin_frozen":0E-18,)"
                 R"("profit":0,)"
                 R"("trade":[{)"
                 R"("trade_fee":-0.000001714113323460,)"
                 R"("fee_asset":"ETH",)"
                 R"("real_profit":0,)"
                 R"("profit":0,)"
                 R"("trade_id":100004090543978,)"
                 R"("id":"100004090543978-1442869343784435712-1",)"
                 R"("trade_volume":1,)"
                 R"("trade_price":2916.96,)"
                 R"("trade_turnover":10.000000000000000000,)"
                 R"("created_at":1764048458767,)"
                 R"("role":"taker")"
                 R"(})"
                 R"(],)"
                 R"("canceled_at":0,)"
                 R"("fee_asset":"ETH",)"
                 R"("uid":"573242943",)"
                 R"("liquidation_type":"0",)"
                 R"("is_tpsl":0,)"
                 R"("real_profit":0,)"
                 R"("self-match-prevent":1,)"
                 R"("canceled_source":null)"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  [[maybe_unused]] json::Orders obj{message, buffers};
}
