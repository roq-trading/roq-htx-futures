/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/parser_2.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("coin_m_perpetual", "[json_positions]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"positions",)"
                 R"("ts":1763811216435,)"
                 R"("event":"init",)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":5,)"
                 R"("direction":"buy",)"
                 R"("last_price":84097.8,)"
                 R"("adl_risk_percent":null)"
                 R"(},{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":5,)"
                 R"("direction":"sell",)"
                 R"("last_price":84097.8,)"
                 R"("adl_risk_percent":null)"
                 R"(},{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":5,)"
                 R"("direction":"buy",)"
                 R"("last_price":2736.21,)"
                 R"("adl_risk_percent":null)"
                 R"(},{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":5,)"
                 R"("direction":"sell",)"
                 R"("last_price":2736.21,)"
                 R"("adl_risk_percent":null)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Positions obj{message, buffers};
  CHECK(obj.op == json::Operator::NOTIFY);
  // parser
  struct Handler final : public json::Parser2::Handler {
    void operator()(Trace<json::Close> const &) override { FAIL(); }
    void operator()(Trace<json::Error2> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Sub> const &) override { FAIL(); }
    void operator()(Trace<json::FundingRate> const &) override { FAIL(); }
    void operator()(Trace<json::Accounts> const &) override { FAIL(); }
    void operator()(Trace<json::Positions> const &event) override {
      found = true;
      auto &[trace_info, positions] = event;
      CHECK(positions.op == json::Operator::NOTIFY);
    };
    void operator()(Trace<json::MatchOrders> const &) override { FAIL(); }
    void operator()(Trace<json::Orders> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser2::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}

TEST_CASE("coin_m_perpetual_order_match", "[json_positions]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"positions.eth-usd",)"
                 R"("ts":1764049813037,)"
                 R"("event":"order.match",)"
                 R"("data":[{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":0,)"
                 R"("available":0,)"
                 R"("frozen":0,)"
                 R"("cost_open":0,)"
                 R"("cost_hold":0,)"
                 R"("profit_unreal":0,)"
                 R"("profit_rate":0,)"
                 R"("profit":0,)"
                 R"("position_margin":0,)"
                 R"("lever_rate":1,)"
                 R"("direction":"buy",)"
                 R"("last_price":2932.19,)"
                 R"("adl_risk_percent":4)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Positions obj{message, buffers};
  CHECK(obj.op == json::Operator::NOTIFY);
  // parser
  struct Handler final : public json::Parser2::Handler {
    void operator()(Trace<json::Close> const &) override { FAIL(); }
    void operator()(Trace<json::Error2> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Sub> const &) override { FAIL(); }
    void operator()(Trace<json::FundingRate> const &) override { FAIL(); }
    void operator()(Trace<json::Accounts> const &) override { FAIL(); }
    void operator()(Trace<json::Positions> const &event) override {
      found = true;
      auto &[trace_info, positions] = event;
      CHECK(positions.op == json::Operator::NOTIFY);
    };
    void operator()(Trace<json::MatchOrders> const &) override { FAIL(); }
    void operator()(Trace<json::Orders> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser2::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}

TEST_CASE("coin_m_perpetual_order_close", "[json_positions]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"positions.eth-usd",)"
                 R"("ts":1764054509308,)"
                 R"("event":"order.close",)"
                 R"("data":[{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("volume":1.000000000000000000,)"
                 R"("available":0E-18,)"
                 R"("frozen":1.000000000000000000,)"
                 R"("cost_open":2916.960000000000153502,)"
                 R"("cost_hold":2916.960000000000153502,)"
                 R"("profit_unreal":-0.000004600890298790,)"
                 R"("profit_rate":-0.001342061296595494,)"
                 R"("profit":-0.000004600890298790,)"
                 R"("position_margin":0.003423625756621292,)"
                 R"("lever_rate":1,)"
                 R"("direction":"sell",)"
                 R"("last_price":2920.88,)"
                 R"("adl_risk_percent":3)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Positions obj{message, buffers};
  CHECK(obj.op == json::Operator::NOTIFY);
  // parser
  struct Handler final : public json::Parser2::Handler {
    void operator()(Trace<json::Close> const &) override { FAIL(); }
    void operator()(Trace<json::Error2> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Sub> const &) override { FAIL(); }
    void operator()(Trace<json::FundingRate> const &) override { FAIL(); }
    void operator()(Trace<json::Accounts> const &) override { FAIL(); }
    void operator()(Trace<json::Positions> const &event) override {
      found = true;
      auto &[trace_info, positions] = event;
      CHECK(positions.op == json::Operator::NOTIFY);
    };
    void operator()(Trace<json::MatchOrders> const &) override { FAIL(); }
    void operator()(Trace<json::Orders> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser2::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}
