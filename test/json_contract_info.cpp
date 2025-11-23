/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/contract_info.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

// note! reduced
TEST_CASE("usdt_m_futures", "[json_contract_info]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USDT",)"
                 R"("contract_size":0.001000000000000000,)"
                 R"("price_tick":0.100000000000000000,)"
                 R"("delivery_date":"",)"
                 R"("delivery_time":"",)"
                 R"("create_date":"20201021",)"
                 R"("contract_status":1,)"
                 R"("adjust":[],)"
                 R"("price_estimated":[],)"
                 R"("settlement_date":"1763798400000",)"
                 R"("support_margin_mode":"all",)"
                 R"("open_type":0,)"
                 R"("business_type":"swap",)"
                 R"("pair":"BTC-USDT",)"
                 R"("contract_type":"swap",)"
                 R"("trade_partition":"USDT")"
                 R"(},{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USDT",)"
                 R"("contract_size":0.010000000000000000,)"
                 R"("price_tick":0.010000000000000000,)"
                 R"("delivery_date":"",)"
                 R"("delivery_time":"",)"
                 R"("create_date":"20201021",)"
                 R"("contract_status":1,)"
                 R"("adjust":[],)"
                 R"("price_estimated":[],)"
                 R"("settlement_date":"1763798400000",)"
                 R"("support_margin_mode":"all",)"
                 R"("open_type":0,)"
                 R"("business_type":"swap",)"
                 R"("pair":"ETH-USDT",)"
                 R"("contract_type":"swap",)"
                 R"("trade_partition":"USDT")"
                 R"(})"
                 R"(],)"
                 R"("ts":1763791421826)"
                 R"(})";
  core::json::BufferStack buffer{8192, 1};
  [[maybe_unused]] json::ContractInfo obj{message, buffer};
}

// note! reduced
TEST_CASE("coin_m_delivery", "[json_contract_info]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC251128",)"
                 R"("contract_type":"this_week",)"
                 R"("contract_size":100.000000000000000000,)"
                 R"("price_tick":0.010000000000000000,)"
                 R"("delivery_date":"20251128",)"
                 R"("delivery_time":"1764316800000",)"
                 R"("create_date":"20251114",)"
                 R"("contract_status":1,)"
                 R"("settlement_time":"1763798400000")"
                 R"(},{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC251205",)"
                 R"("contract_type":"next_week",)"
                 R"("contract_size":100.000000000000000000,)"
                 R"("price_tick":0.010000000000000000,)"
                 R"("delivery_date":"20251205",)"
                 R"("delivery_time":"1764921600000",)"
                 R"("create_date":"20251121",)"
                 R"("contract_status":1,)"
                 R"("settlement_time":"1763798400000")"
                 R"(})"
                 R"(],)"
                 R"("ts":1763791626928)"
                 R"(})";
  core::json::BufferStack buffer{8192, 1};
  [[maybe_unused]] json::ContractInfo obj{message, buffer};
}

// note! reduced
TEST_CASE("coin_m_perpetual", "[json_contract_info]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("contract_size":100.000000000000000000,)"
                 R"("price_tick":0.100000000000000000,)"
                 R"("delivery_time":"",)"
                 R"("create_date":"20200325",)"
                 R"("contract_status":1,)"
                 R"("settlement_date":"1763798400000")"
                 R"(},{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("contract_size":10.000000000000000000,)"
                 R"("price_tick":0.010000000000000000,)"
                 R"("delivery_time":"",)"
                 R"("create_date":"20200403",)"
                 R"("contract_status":1,)"
                 R"("settlement_date":"1763798400000")"
                 R"(})"
                 R"(],)"
                 R"("ts":1763791707461)"
                 R"(})";
  core::json::BufferStack buffer{8192, 1};
  [[maybe_unused]] json::ContractInfo obj{message, buffer};
}
