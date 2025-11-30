/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/parser.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_premium_index_simple_swap", "[json_premium_index]") {
  auto message = R"({)"
                 R"("ch":"market.BTC-USD.premium_index.1min",)"
                 R"("ts":1642657680747,)"
                 R"("tick":{)"
                 R"("id":1642657680,)"
                 R"("open":"-0.0002871886442248",)"
                 R"("close":"-0.0002871886442248",)"
                 R"("high":"-0.0002871886442248",)"
                 R"("low":"-0.0002871886442248",)"
                 R"("amount":"0",)"
                 R"("vol":"0",)"
                 R"("count":"0")"
                 R"(})"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::PremiumIndex obj{message, buffers};
  CHECK(obj.ch == "market.BTC-USD.premium_index.1min"sv);
  CHECK(obj.ts == 1642657680747ms);
  auto &tick = obj.tick;
  CHECK(tick.id == 1642657680);
  CHECK(tick.id == 1642657680);
  CHECK(tick.open == -0.0002871886442248_a);
  CHECK(tick.close == -0.0002871886442248_a);
  CHECK(tick.high == -0.0002871886442248_a);
  CHECK(tick.low == -0.0002871886442248_a);
  CHECK(tick.amount == 0.0_a);
  CHECK(tick.vol == 0.0_a);
  CHECK(tick.count == 0.0_a);
  // parser
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Subbed> const &) override { FAIL(); }
    void operator()(Trace<json::BBO> const &) override { FAIL(); }
    void operator()(Trace<json::Depth> const &) override { FAIL(); }
    void operator()(Trace<json::Trade> const &) override { FAIL(); }
    void operator()(Trace<json::Detail> const &) override { FAIL(); }
    void operator()(Trace<json::EstimatedRate> const &) override { FAIL(); }
    void operator()(Trace<json::PremiumIndex> const &event) override {
      found = true;
      auto &[trace_info, premium_index] = event;
      CHECK(premium_index.ch == "market.BTC-USD.premium_index.1min"sv);
    }
    void operator()(Trace<json::Basis> const &) override { FAIL(); }
    void operator()(Trace<json::Index> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}
