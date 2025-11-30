/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/parser.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("swap", "[json_estimated_rate]") {
  auto message = R"({)"
                 R"("ch":"market.BTC-USD.estimated_rate.1min",)"
                 R"("ts":1642658280847,)"
                 R"("tick":{)"
                 R"("id":1642658280,)"
                 R"("open":"0.0001",)"
                 R"("close":"0.0001",)"
                 R"("high":"0.0001",)"
                 R"("low":"0.0001",)"
                 R"("amount":"0",)"
                 R"("vol":"0",)"
                 R"("count":"0")"
                 R"(})"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::EstimatedRate obj{message, buffers};
  CHECK(obj.ch == "market.BTC-USD.estimated_rate.1min"sv);
  // parser
  struct Handler final : public json::Parser::Handler {
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Error> const &) override { FAIL(); }
    void operator()(Trace<json::Subbed> const &) override { FAIL(); }
    void operator()(Trace<json::BBO> const &) override { FAIL(); }
    void operator()(Trace<json::Depth> const &) override { FAIL(); }
    void operator()(Trace<json::Trade> const &) override { FAIL(); }
    void operator()(Trace<json::Detail> const &) override { FAIL(); }
    void operator()(Trace<json::EstimatedRate> const &event) override {
      found = true;
      auto &[trace_info, estimated_rate] = event;
      CHECK(estimated_rate.ch == "market.BTC-USD.estimated_rate.1min"sv);
    }
    void operator()(Trace<json::PremiumIndex> const &) override { FAIL(); }
    void operator()(Trace<json::Basis> const &) override { FAIL(); }
    void operator()(Trace<json::Index> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}
