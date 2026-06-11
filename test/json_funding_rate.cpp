/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/protocol/json/funding_rate.hpp"
// #include "roq/htx_futures/protocol/json/parser.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("swap", "[json_funding_rate]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"public.BTC-USDT.funding_rate",)"
                 R"("ts":1643960260995,)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USDT",)"
                 R"("fee_asset":"USDT",)"
                 R"("funding_time":"1643960220000",)"
                 R"("funding_rate":"0.000087224669991579",)"
                 R"("estimated_rate":"-0.000065319255924193",)"
                 R"("settlement_time":"1643961600000",)"
                 R"("trade_partition":"USDT")"
                 R"(})"
                 R"(])"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  protocol::json::FundingRate obj{message, buffers};
  CHECK(obj.op == protocol::json::Operator::NOTIFY);
  CHECK(obj.topic == "public.BTC-USDT.funding_rate"sv);
  CHECK(obj.ts == 1643960260995ms);
  auto &data = obj.data;
  CHECK(std::size(data) == 1);
  auto &d0 = data[0];
  CHECK(d0.symbol == "BTC"sv);
  CHECK(d0.contract_code == "BTC-USDT"sv);
  CHECK(d0.fee_asset == "USDT"sv);
  CHECK(d0.funding_time == 1643960220000ms);
  CHECK(d0.funding_rate == 0.000087224669991579_a);
  CHECK(d0.estimated_rate == -0.000065319255924193_a);
  CHECK(d0.settlement_time == 1643961600000ms);
  CHECK(d0.trade_partition == "USDT"sv);
  // parser
  /*
  struct Handler final : public protocol::json::Parser::Handler {
    void operator()(Trace<protocol::json::Ping> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::Error> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::Subbed> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::BBO> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::Depth> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::Trade> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::Detail> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::EstimatedRate> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::PremiumIndex> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::Basis> const &) override { FAIL(); }
    void operator()(Trace<protocol::json::Index> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = protocol::json::Parser::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
  */
}
