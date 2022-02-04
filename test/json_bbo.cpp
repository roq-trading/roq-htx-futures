/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/bbo.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

TEST(json_bbo, simple) {
  auto message = R"({)"
                 R"("ch":"market.TRX211224.bbo",)"
                 R"("ts":1639583658324,)"
                 R"("tick":{)"
                 R"("mrid":131671795982,)"
                 R"("id":1639583658,)"
                 R"("bid":[0.08308,71],)"
                 R"("ask":[0.08337,247],)"
                 R"("ts":1639583658324,)"
                 R"("version":131671795982,)"
                 R"("ch":"market.TRX211224.bbo")"
                 R"(})"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::BBO>(message, buffer_);
  EXPECT_EQ(obj.ch, "market.TRX211224.bbo"sv);
  EXPECT_EQ(obj.ts, 1639583658324ms);
  auto &tick = obj.tick;
  EXPECT_EQ(tick.mrid, 131671795982);
  EXPECT_EQ(tick.id, 1639583658);
  EXPECT_DOUBLE_EQ(tick.bid.price, 0.08308);
  EXPECT_DOUBLE_EQ(tick.bid.vol, 71.0);
  EXPECT_DOUBLE_EQ(tick.ask.price, 0.08337);
  EXPECT_DOUBLE_EQ(tick.ask.vol, 247.0);
  EXPECT_EQ(tick.ts, 1639583658324ms);
  EXPECT_EQ(tick.version, 131671795982);
  EXPECT_EQ(tick.ch, "market.TRX211224.bbo"sv);
}
