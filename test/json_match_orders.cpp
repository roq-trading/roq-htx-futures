/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/match_orders.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("coin_m_perpetual_create", "[json_match_orders]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"matchOrders.btc-usd",)"
                 R"("ts":1763891916920,)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("status":3,)"
                 R"("order_id":1442212759625592832,)"
                 R"("order_id_str":"1442212759625592832",)"
                 R"("client_order_id":563231976427594,)"
                 R"("order_type":1,)"
                 R"("created_at":1763891916906,)"
                 R"("trade":[],)"
                 R"("uid":"573242943",)"
                 R"("volume":1,)"
                 R"("trade_volume":0,)"
                 R"("direction":"sell",)"
                 R"("offset":"open",)"
                 R"("lever_rate":1,)"
                 R"("price":200000.0,)"
                 R"("order_source":"api",)"
                 R"("order_price_type":"limit",)"
                 R"("is_tpsl":0,)"
                 R"("self-match-prevent":1)"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  [[maybe_unused]] json::MatchOrders obj{message, buffers};
}

TEST_CASE("coin_m_perpetual_cancel", "[json_match_orders]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"matchOrders.btc-usd",)"
                 R"("ts":1763892991576,)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("status":7,)"
                 R"("order_id":1442212759625592832,)"
                 R"("order_id_str":"1442212759625592832",)"
                 R"("client_order_id":563231976427594,)"
                 R"("order_type":2,)"
                 R"("created_at":1763892991572,)"
                 R"("trade":[],)"
                 R"("uid":"573242943",)"
                 R"("volume":1,)"
                 R"("trade_volume":0,)"
                 R"("direction":"sell",)"
                 R"("offset":"open",)"
                 R"("lever_rate":1,)"
                 R"("price":200000.0,)"
                 R"("order_source":"api",)"
                 R"("order_price_type":"limit",)"
                 R"("is_tpsl":0,)"
                 R"("self-match-prevent":1)"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  [[maybe_unused]] json::MatchOrders obj{message, buffers};
}
