/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/funding_rate.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_funding_rate_simple_swap", "[json_funding_rate]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"public.BTC-USDT.funding_rate",)"
                 R"("ts":1643960260995,)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USDT",)"
                 R"("fee_asset":"USDT",)"
                 R"("funding_time":"1643960220000",)"
                 R"("funding_rate":"0.000087224669991579",)"
                 R"("estimated_rate":"-0.000065319255924193",)"
                 R"("settlement_time":"1643961600000",)"
                 R"("trade_partition":"USDT")"
                 R"(})"
                 R"(])"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::FundingRate>(message, buffer_);
  CHECK(obj.op == json::Operator::NOTIFY);
  CHECK(obj.topic == "public.BTC-USDT.funding_rate"sv);
  CHECK(obj.ts == 1643960260995ms);
  auto &data = obj.data;
  CHECK(std::size(data) == 1);
  auto &d0 = data[0];
  CHECK(d0.symbol == "BTC"sv);
  CHECK(d0.contract_code == "BTC-USDT"sv);
  CHECK(d0.fee_asset == "USDT"sv);
  CHECK(d0.funding_time == 1643960220000ms);
  CHECK(d0.funding_rate == 0.000087224669991579_a);
  CHECK(d0.estimated_rate == -0.000065319255924193_a);
  CHECK(d0.settlement_time == 1643961600000ms);
  CHECK(d0.trade_partition == "USDT"sv);
}
