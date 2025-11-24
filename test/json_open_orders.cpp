/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/open_orders.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("coin_m_perpetual_empty", "[json_open_orders]") {
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
  core::json::BufferStack buffer{8192, 2};
  [[maybe_unused]] json::OpenOrders obj{message, buffer};
}

TEST_CASE("coin_m_perpetual_simple", "[json_open_orders]") {
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
  core::json::BufferStack buffer{8192, 2};
  [[maybe_unused]] json::OpenOrders obj{message, buffer};
}
