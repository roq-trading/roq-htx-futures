/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/huobi_futures/json/bbo.hpp"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_bbo_simple", "[json_bbo]") {
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
  CHECK(obj.ch == "market.TRX211224.bbo"sv);
  CHECK(obj.ts == 1639583658324ms);
  auto &tick = obj.tick;
  CHECK(tick.mrid == 131671795982);
  CHECK(tick.id == 1639583658);
  CHECK(tick.bid.price == 0.08308_a);
  CHECK(tick.bid.vol == 71.0_a);
  CHECK(tick.ask.price == 0.08337_a);
  CHECK(tick.ask.vol == 247.0_a);
  CHECK(tick.ts == 1639583658324ms);
  CHECK(tick.version == 131671795982);
  CHECK(tick.ch == "market.TRX211224.bbo"sv);
}
