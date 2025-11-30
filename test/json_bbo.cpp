/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/parser.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("simple", "[json_bbo]") {
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
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::BBO obj{message, buffers};
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
  // parser
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Subbed> const &) override { FAIL(); }
    void operator()(Trace<json::BBO> const &event) override {
      found = true;
      auto &[trace_info, bbo] = event;
      CHECK(bbo.ch == "market.TRX211224.bbo"sv);
    }
    void operator()(Trace<json::Depth> const &) override { FAIL(); }
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
