/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/huobi_futures/json/detail.hpp"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_detail_simple_inverse", "[json_detail]") {
  auto message = R"({)"
                 R"("ch":"market.FIL211231.detail",)"
                 R"("ts":1639628009780,)"
                 R"("tick":{)"
                 R"("id":1639627980,)"
                 R"("mrid":149495401281,)"
                 R"("open":35.714,)"
                 R"("close":37.795,)"
                 R"("high":38.901,)"
                 R"("low":35.641,)"
                 R"("amount":68725.1520427579159023020649636609479771038,)"
                 R"("vol":255300,)"
                 R"("count":4172,)"
                 R"("ask":[37.809,145],)"
                 R"("bid":[37.783,4])"
                 R"(})"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::Detail>(message, buffer_);
  CHECK(obj.ch == "market.FIL211231.detail"sv);
  CHECK(obj.ts == 1639628009780ms);
  auto &tick = obj.tick;
  CHECK(tick.id == 1639627980);
  CHECK(tick.mrid == 149495401281);
  CHECK(tick.open == 35.714_a);
  CHECK(tick.close == 37.795_a);
  CHECK(tick.high == 38.901_a);
  CHECK(tick.low == 35.641_a);
  CHECK(tick.amount == 68725.1520427579159023020649636609479771038_a);
  CHECK(tick.vol == 255300.0_a);
  CHECK(tick.count == 4172.0_a);
  auto &ask = tick.ask;
  CHECK(ask.price == 37.809_a);
  CHECK(ask.vol == 145.0_a);
  auto &bid = tick.bid;
  CHECK(bid.price == 37.783_a);
  CHECK(bid.vol == 4.0_a);
}

TEST_CASE("json_detail_simple_linear", "[json_detail]") {
  auto message = R"({)"
                 R"("ch":"market.WOO-USDT.detail",)"
                 R"("ts":1640775846213,)"
                 R"("tick":{)"
                 R"("id":1640775840,)"
                 R"("mrid":38292289192,)"
                 R"("open":0.99934,)"
                 R"("close":0.89934,)"
                 R"("high":1.00166,)"
                 R"("low":0.88366,)"
                 R"("amount":738940,)"
                 R"("vol":73894,)"
                 R"("trade_turnover":683194.497,)"
                 R"("count":4256,)"
                 R"("ask":[0.90157,10],)"
                 R"("bid":[0.89738,235])"
                 R"(})"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::Detail>(message, buffer_);
  CHECK(obj.ch == "market.WOO-USDT.detail"sv);
  CHECK(obj.ts == 1640775846213ms);
  auto &tick = obj.tick;
  CHECK(tick.id == 1640775840);
  CHECK(tick.mrid == 38292289192);
  CHECK(tick.open == 0.99934_a);
  CHECK(tick.close == 0.89934_a);
  CHECK(tick.high == 1.00166_a);
  CHECK(tick.low == 0.88366_a);
  CHECK(tick.amount == 738940.0_a);
  CHECK(tick.vol == 73894.0_a);
  CHECK(tick.trade_turnover == 683194.497_a);
  CHECK(tick.count == 4256.0_a);
  auto &ask = tick.ask;
  CHECK(ask.price == 0.90157_a);
  CHECK(ask.vol == 10.0_a);
  auto &bid = tick.bid;
  CHECK(bid.price == 0.89738_a);
  CHECK(bid.vol == 235.0_a);
}
