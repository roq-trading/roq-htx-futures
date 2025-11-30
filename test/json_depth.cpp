/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/parser.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

// note! reduced
TEST_CASE("simple", "[json_depth]") {
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
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Depth obj{message, buffers};
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
  // parser
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Subbed> const &) override { FAIL(); }
    void operator()(Trace<json::BBO> const &) override { FAIL(); }
    void operator()(Trace<json::Depth> const &event) override {
      found = true;
      auto &[trace_info, depth] = event;
      CHECK(depth.ch == "market.FIL211231.depth.size_150.high_freq"sv);
    }
    void operator()(Trace<json::Trade> const &) override { FAIL(); }
    void operator()(Trace<json::Detail> const &) override { FAIL(); }
    void operator()(Trace<json::EstimatedRate> const &) override { FAIL(); }
    void operator()(Trace<json::PremiumIndex> const &) override { FAIL(); }
    void operator()(Trace<json::Basis> const &) override { FAIL(); }
    void operator()(Trace<json::Index> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}
