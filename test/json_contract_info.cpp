/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/contract_info.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

// note! reduced
TEST(json_contract_info, simple) {
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
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::ContractInfo>(message, buffer_);
}
