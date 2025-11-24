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
