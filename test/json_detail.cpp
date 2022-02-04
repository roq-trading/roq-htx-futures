/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/detail.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

TEST(json_detail, simple_inverse) {
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
  EXPECT_EQ(obj.ch, "market.FIL211231.detail"sv);
  EXPECT_EQ(obj.ts, 1639628009780ms);
  auto &tick = obj.tick;
  EXPECT_EQ(tick.id, 1639627980);
  EXPECT_EQ(tick.mrid, 149495401281);
  EXPECT_DOUBLE_EQ(tick.open, 35.714);
  EXPECT_DOUBLE_EQ(tick.close, 37.795);
  EXPECT_DOUBLE_EQ(tick.high, 38.901);
  EXPECT_DOUBLE_EQ(tick.low, 35.641);
  EXPECT_DOUBLE_EQ(tick.amount, 68725.1520427579159023020649636609479771038);
  EXPECT_DOUBLE_EQ(tick.vol, 255300.0);
  EXPECT_DOUBLE_EQ(tick.count, 4172.0);
  auto &ask = tick.ask;
  EXPECT_DOUBLE_EQ(ask.price, 37.809);
  EXPECT_DOUBLE_EQ(ask.vol, 145.0);
  auto &bid = tick.bid;
  EXPECT_DOUBLE_EQ(bid.price, 37.783);
  EXPECT_DOUBLE_EQ(bid.vol, 4.0);
}

TEST(json_detail, simple_linear) {
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
  EXPECT_EQ(obj.ch, "market.WOO-USDT.detail"sv);
  EXPECT_EQ(obj.ts, 1640775846213ms);
  auto &tick = obj.tick;
  EXPECT_EQ(tick.id, 1640775840);
  EXPECT_EQ(tick.mrid, 38292289192);
  EXPECT_DOUBLE_EQ(tick.open, 0.99934);
  EXPECT_DOUBLE_EQ(tick.close, 0.89934);
  EXPECT_DOUBLE_EQ(tick.high, 1.00166);
  EXPECT_DOUBLE_EQ(tick.low, 0.88366);
  EXPECT_DOUBLE_EQ(tick.amount, 738940.0);
  EXPECT_DOUBLE_EQ(tick.vol, 73894.0);
  EXPECT_DOUBLE_EQ(tick.trade_turnover, 683194.497);
  EXPECT_DOUBLE_EQ(tick.count, 4256.0);
  auto &ask = tick.ask;
  EXPECT_DOUBLE_EQ(ask.price, 0.90157);
  EXPECT_DOUBLE_EQ(ask.vol, 10.0);
  auto &bid = tick.bid;
  EXPECT_DOUBLE_EQ(bid.price, 0.89738);
  EXPECT_DOUBLE_EQ(bid.vol, 235.0);
}
