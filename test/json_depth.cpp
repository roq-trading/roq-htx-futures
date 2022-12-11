/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/huobi_futures/json/depth.hpp"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

// note! reduced
TEST_CASE("json_depth_simple", "[json_depth]") {
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
  CHECK(obj.ch == "market.FIL211231.depth.size_150.high_freq"sv);
  auto &tick = obj.tick;
  auto &asks = tick.asks;
  REQUIRE(std::size(asks) == 2);
  auto &a0 = asks[0];
  CHECK(a0.price == 37.722_a);
  CHECK(a0.vol == 2.0_a);
  auto &a1 = asks[1];
  CHECK(a1.price == 37.735_a);
  CHECK(a1.vol == 145.0_a);
  auto &bids = tick.bids;
  REQUIRE(std::size(bids) == 2);
  auto &b0 = bids[0];
  CHECK(b0.price == 37.71_a);
  CHECK(b0.vol == 145.0_a);
  auto &b1 = bids[1];
  CHECK(b1.price == 37.709_a);
  CHECK(b1.vol == 170.0_a);
  CHECK(tick.ch == "market.FIL211231.depth.size_150.high_freq"sv);
  CHECK(tick.event == json::Event::SNAPSHOT);
  CHECK(tick.id == 149496559186);
  CHECK(tick.mrid == 149496559186);
  CHECK(tick.ts == 1639630955318ms);
  CHECK(tick.version == 195613528);
  CHECK(obj.ts == 1639630955318ms);
}
