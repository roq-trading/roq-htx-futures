/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/json/parser_2.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("coin_m_perpetual_init", "[json_accounts]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"accounts",)"
                 R"("ts":1763809542628,)"
                 R"("event":"init",)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("margin_balance":0.001000000000000000,)"
                 R"("margin_static":0.00100000,)"
                 R"("margin_position":0E-18,)"
                 R"("margin_frozen":0,)"
                 R"("margin_available":0.001000000000000000,)"
                 R"("profit_real":0,)"
                 R"("profit_unreal":0E-18,)"
                 R"("withdraw_available":0.001000000000000000,)"
                 R"("risk_rate":null,)"
                 R"("liquidation_price":null,)"
                 R"("lever_rate":5,)"
                 R"("adjust_factor":0.025000000000000000)"
                 R"(},{)"
                 R"("symbol":"ETH",)"
                 R"("contract_code":"ETH-USD",)"
                 R"("margin_balance":0.010000000000000000,)"
                 R"("margin_static":0.01000000,)"
                 R"("margin_position":0E-18,)"
                 R"("margin_frozen":0,)"
                 R"("margin_available":0.010000000000000000,)"
                 R"("profit_real":0,)"
                 R"("profit_unreal":0E-18,)"
                 R"("withdraw_available":0.010000000000000000,)"
                 R"("risk_rate":null,)"
                 R"("liquidation_price":null,)"
                 R"("lever_rate":5,)"
                 R"("adjust_factor":0.025000000000000000)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Accounts obj{message, buffers};
  CHECK(obj.op == json::Operator::NOTIFY);
  // parser
  struct Handler final : public json::Parser2::Handler {
    void operator()(Trace<json::Close> const &) override { FAIL(); }
    void operator()(Trace<json::Error2> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Sub> const &) override { FAIL(); }
    void operator()(Trace<json::FundingRate> const &) override { FAIL(); }
    void operator()(Trace<json::Accounts> const &event) override {
      found = true;
      auto &[trace_info, accounts] = event;
      CHECK(accounts.op == json::Operator::NOTIFY);
    };
    void operator()(Trace<json::Positions> const &) override { FAIL(); }
    void operator()(Trace<json::MatchOrders> const &) override { FAIL(); }
    void operator()(Trace<json::Orders> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser2::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}

TEST_CASE("coin_m_perpetual_order_open", "[json_accounts]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"accounts.btc-usd",)"
                 R"("ts":1763959722536,)"
                 R"("event":"order.open",)"
                 R"("data":[{)"
                 R"("symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("margin_balance":0.001000000000000000,)"
                 R"("margin_static":0.00100000,)"
                 R"("margin_position":0E-18,)"
                 R"("margin_frozen":0.000500000000000000,)"
                 R"("margin_available":0.000500000000000000,)"
                 R"("profit_real":0,)"
                 R"("profit_unreal":0E-18,)"
                 R"("withdraw_available":0.000500000000000000,)"
                 R"("risk_rate":1.995000000000000000,)"
                 R"("liquidation_price":null,)"
                 R"("lever_rate":1,)"
                 R"("adjust_factor":0.005000000000000000)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Accounts obj{message, buffers};
  CHECK(obj.op == json::Operator::NOTIFY);
  // parser
  struct Handler final : public json::Parser2::Handler {
    void operator()(Trace<json::Close> const &) override { FAIL(); }
    void operator()(Trace<json::Error2> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Sub> const &) override { FAIL(); }
    void operator()(Trace<json::FundingRate> const &) override { FAIL(); }
    void operator()(Trace<json::Accounts> const &event) override {
      found = true;
      auto &[trace_info, accounts] = event;
      CHECK(accounts.op == json::Operator::NOTIFY);
    };
    void operator()(Trace<json::Positions> const &) override { FAIL(); }
    void operator()(Trace<json::MatchOrders> const &) override { FAIL(); }
    void operator()(Trace<json::Orders> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser2::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}

TEST_CASE("coin_m_perpetual_order_cancel", "[json_accounts]") {
  auto message = R"({)"
                 R"("op":"notify",)"
                 R"("topic":"accounts.btc-usd",)"
                 R"("ts":1763960170327,)"
                 R"("event":"order.cancel",)"
                 R"("data":[{"symbol":"BTC",)"
                 R"("contract_code":"BTC-USD",)"
                 R"("margin_balance":0.001000000000000000,)"
                 R"("margin_static":0.00100000,)"
                 R"("margin_position":0E-18,)"
                 R"("margin_frozen":0E-18,)"
                 R"("margin_available":0.001000000000000000,)"
                 R"("profit_real":0,)"
                 R"("profit_unreal":0E-18,)"
                 R"("withdraw_available":0.001000000000000000,)"
                 R"("risk_rate":null,)"
                 R"("liquidation_price":null,)"
                 R"("lever_rate":1,)"
                 R"("adjust_factor":0.005000000000000000)"
                 R"(})"
                 R"(],)"
                 R"("uid":"573242943")"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Accounts obj{message, buffers};
  CHECK(obj.op == json::Operator::NOTIFY);
  // parser
  struct Handler final : public json::Parser2::Handler {
    void operator()(Trace<json::Close> const &) override { FAIL(); }
    void operator()(Trace<json::Error2> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
    void operator()(Trace<json::Sub> const &) override { FAIL(); }
    void operator()(Trace<json::FundingRate> const &) override { FAIL(); }
    void operator()(Trace<json::Accounts> const &event) override {
      found = true;
      auto &[trace_info, accounts] = event;
      CHECK(accounts.op == json::Operator::NOTIFY);
    };
    void operator()(Trace<json::Positions> const &) override { FAIL(); }
    void operator()(Trace<json::MatchOrders> const &) override { FAIL(); }
    void operator()(Trace<json::Orders> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser2::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}
