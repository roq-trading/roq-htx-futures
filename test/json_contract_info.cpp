/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/huobi_futures/json/contract_info.hpp"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

// note! reduced
TEST_CASE("json_contract_info_simple_inverse", "[json_contract_info]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC211217",)"
                 R"("contract_type":"this_week",)"
                 R"("contract_size":100.000000000000000000,)"
                 R"("price_tick":0.010000000000000000,)"
                 R"("delivery_date":"20211217",)"
                 R"("delivery_time":"1639728000000",)"
                 R"("create_date":"20211203",)"
                 R"("contract_status":1,)"
                 R"("settlement_time":"1639641600000")"
                 R"(},{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC211224",)"
                 R"("contract_type":"next_week",)"
                 R"("contract_size":100.000000000000000000,)"
                 R"("price_tick":0.010000000000000000,)"
                 R"("delivery_date":"20211224",)"
                 R"("delivery_time":"1640332800000",)"
                 R"("create_date":"20211210",)"
                 R"("contract_status":1,)"
                 R"("settlement_time":"1639641600000")"
                 R"(})"
                 R"(],)"
                 R"("ts":1639583414002)"
                 R"(})";
  std::vector<std::byte> buffer(8192);
  json::ContractInfo obj{message, buffer};
  CHECK(obj.status == "ok"sv);
  auto &data = obj.data;
  REQUIRE(std::size(data) == 2);
  auto &d0 = data[0];
  CHECK(d0.symbol == "BTC"sv);
  CHECK(d0.contract_code == "BTC211217"sv);
  CHECK(d0.contract_type == "this_week"sv);
  CHECK(d0.contract_size == 100.0_a);
  CHECK(d0.price_tick == 0.01_a);
  // CHECK(d0.delivery_data == "20211217"sv);
  CHECK(d0.delivery_time == 1639728000000ms);
  // EXPECT_EQ(d0.create_date, "20211203";
  CHECK(d0.contract_status == 1);
  CHECK(d0.settlement_time == 1639641600000ms);
  auto &d1 = data[1];
  CHECK(d1.symbol == "BTC"sv);
  CHECK(d1.contract_code == "BTC211224"sv);
  CHECK(d1.contract_type == "next_week"sv);
  CHECK(d1.contract_size == 100.0_a);
  CHECK(d1.price_tick == 0.01_a);
  // CHECK(d1.delivery_data == "20211224"sv);
  CHECK(d1.delivery_time == 1640332800000ms);
  // EXPECT_EQ(d1.create_date, "20211210";
  CHECK(d1.contract_status == 1);
  CHECK(d1.settlement_time == 1639641600000ms);
  CHECK(obj.ts == 1639583414002ms);
}

// note! reduced
TEST_CASE("json_contract_info_simple_linear", "[json_contract_info]") {
  auto message = R"({)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USDT",)"
                 R"("contract_size":0.001000000000000000,)"
                 R"("price_tick":0.100000000000000000,)"
                 R"("delivery_date":"",)"
                 R"("delivery_time":"",)"
                 R"("create_date":"20201021",)"
                 R"("contract_status":1,)"
                 R"("settlement_date":"1640793600000",)"
                 R"("support_margin_mode":"all",)"
                 R"("business_type":"swap",)"
                 R"("pair":"BTC-USDT",)"
                 R"("contract_type":"swap")"
                 R"(},{)"
                 R"("symbol":"BAT",)"
                 R"("contract_code":"BAT-USDT",)"
                 R"("contract_size":10.000000000000000000,)"
                 R"("price_tick":0.000100000000000000,)"
                 R"("delivery_date":"",)"
                 R"("delivery_time":"",)"
                 R"("create_date":"20210120",)"
                 R"("contract_status":3,)"
                 R"("settlement_date":"1640793600000",)"
                 R"("support_margin_mode":"isolated",)"
                 R"("business_type":"swap",)"
                 R"("pair":"BAT-USDT",)"
                 R"("contract_type":"swap")"
                 R"(},{)"
                 R"("symbol":"EGLD",)"
                 R"("contract_code":"EGLD-USDT",)"
                 R"("contract_size":0.010000000000000000,)"
                 R"("price_tick":0.010000000000000000,)"
                 R"("delivery_date":"",)"
                 R"("delivery_time":"",)"
                 R"("create_date":"20210923",)"
                 R"("contract_status":3,)"
                 R"("settlement_date":"1640793600000",)"
                 R"("support_margin_mode":"isolated",)"
                 R"("business_type":"swap",)"
                 R"("pair":"EGLD-USDT",)"
                 R"("contract_type":"swap")"
                 R"(})"
                 R"(],)"
                 R"("ts":1640774191619)"
                 R"(})";
  std::vector<std::byte> buffer(8192);
  [[maybe_unused]] json::ContractInfo obj{message, buffer};
}
