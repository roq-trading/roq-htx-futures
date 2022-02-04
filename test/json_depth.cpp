/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/depth.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

// note! reduced
TEST(json_depth, simple) {
  auto message = R"({)"
                 R"("ch":"market.FIL211231.depth.size_150.high_freq",)"
                 R"("tick":{)"
                 R"("asks":[)"
                 R"([37.722,2],)"
                 R"([37.735,145])"
                 R"(],)"
                 R"("bids":[)"
                 R"([37.71,145],)"
                 R"([37.709,170])"
                 R"(],)"
                 R"("ch":"market.FIL211231.depth.size_150.high_freq",)"
                 R"("event":"snapshot",)"
                 R"("id":149496559186,)"
                 R"("mrid":149496559186,)"
                 R"("ts":1639630955318,)"
                 R"("version":195613528)"
                 R"(},)"
                 R"("ts":1639630955318)"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::Depth>(message, buffer_);
  EXPECT_EQ(obj.ch, "market.FIL211231.depth.size_150.high_freq"sv);
  auto &tick = obj.tick;
  auto &asks = tick.asks;
  ASSERT_EQ(std::size(asks), 2);
  auto &a0 = asks[0];
  EXPECT_DOUBLE_EQ(a0.price, 37.722);
  EXPECT_DOUBLE_EQ(a0.vol, 2.0);
  auto &a1 = asks[1];
  EXPECT_DOUBLE_EQ(a1.price, 37.735);
  EXPECT_DOUBLE_EQ(a1.vol, 145.0);
  auto &bids = tick.bids;
  ASSERT_EQ(std::size(bids), 2);
  auto &b0 = bids[0];
  EXPECT_DOUBLE_EQ(b0.price, 37.71);
  EXPECT_DOUBLE_EQ(b0.vol, 145.0);
  auto &b1 = bids[1];
  EXPECT_DOUBLE_EQ(b1.price, 37.709);
  EXPECT_DOUBLE_EQ(b1.vol, 170.0);
  EXPECT_EQ(tick.ch, "market.FIL211231.depth.size_150.high_freq"sv);
  EXPECT_EQ(tick.event, json::Event::SNAPSHOT);
  EXPECT_EQ(tick.id, 149496559186);
  EXPECT_EQ(tick.mrid, 149496559186);
  EXPECT_EQ(tick.ts, 1639630955318ms);
  EXPECT_EQ(tick.version, 195613528);
  EXPECT_EQ(obj.ts, 1639630955318ms);
}
