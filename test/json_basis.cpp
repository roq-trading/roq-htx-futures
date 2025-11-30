/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/parser.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("swap", "[json_basis]") {
  auto message = R"({)"
                 R"("ch":"market.WOO-USDT.basis.1min.open",)"
                 R"("ts":1642659617542,)"
                 R"("tick":{)"
                 R"("id":1642659600,)"
                 R"("index_price":"0.8645621602666667",)"
                 R"("contract_price":"0.851",)"
                 R"("basis":"-0.0135621602666667",)"
                 R"("basis_rate":"-0.0156867382010838518425939816818336619"})"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Basis obj{message, buffers};
  CHECK(obj.ch == "market.WOO-USDT.basis.1min.open"sv);
  CHECK(obj.ts == 1642659617542ms);
  auto &tick = obj.tick;
  CHECK(tick.id == 1642659600);
  CHECK(tick.index_price == 0.8645621602666667_a);
  CHECK(tick.contract_price == 0.851_a);
  CHECK(tick.basis == -0.0135621602666667_a);
  CHECK(tick.basis_rate == -0.0156867382010838518425939816818336619_a);
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
    void operator()(Trace<json::PremiumIndex> const &) override { FAIL(); }
    void operator()(Trace<json::Basis> const &event) override {
      found = true;
      auto &[trace_info, basis] = event;
      CHECK(basis.ch == "market.WOO-USDT.basis.1min.open"sv);
    }
    void operator()(Trace<json::Index> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}
