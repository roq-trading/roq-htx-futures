/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/parser_2.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("simple", "[json_auth]") {
  auto message = R"({)"
                 R"("op":"auth",)"
                 R"("type":"api",)"
                 R"("err-code":0,)"
                 R"("ts":1763807051526,)"
                 R"("data":{)"
                 R"("user-id":"57324294")"
                 R"(})"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Auth obj{message};
  CHECK(obj.op == json::Operator::AUTH);
  // parser
  struct Handler final : public json::Parser2::Handler {
    void operator()(Trace<json::Close> const &) override { FAIL(); }
    void operator()(Trace<json::Error2> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Auth> const &event) override {
      found = true;
      auto &[trace_info, auth] = event;
      CHECK(auth.op == json::Operator::AUTH);
    };
    void operator()(Trace<json::Sub> const &) override { FAIL(); }
    void operator()(Trace<json::FundingRate> const &) override { FAIL(); }
    void operator()(Trace<json::Accounts> const &) override { FAIL(); }
    void operator()(Trace<json::Positions> const &) override { FAIL(); }
    void operator()(Trace<json::MatchOrders> const &) override { FAIL(); }
    void operator()(Trace<json::Orders> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser2::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}

TEST_CASE("failure", "[json_auth]") {
  auto message = R"({)"
                 R"("op":"auth",)"
                 R"("type":"api",)"
                 R"("ts":1763810619267,)"
                 R"("err-code":2003,)"
                 R"("err-msg":"Verification failure [校验失败]")"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Auth obj{message};
  CHECK(obj.op == json::Operator::AUTH);
  // parser
  struct Handler final : public json::Parser2::Handler {
    void operator()(Trace<json::Close> const &) override { FAIL(); }
    void operator()(Trace<json::Error2> const &) override { FAIL(); }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Auth> const &event) override {
      found = true;
      auto &[trace_info, auth] = event;
      CHECK(auth.op == json::Operator::AUTH);
    };
    void operator()(Trace<json::Sub> const &) override { FAIL(); }
    void operator()(Trace<json::FundingRate> const &) override { FAIL(); }
    void operator()(Trace<json::Accounts> const &) override { FAIL(); }
    void operator()(Trace<json::Positions> const &) override { FAIL(); }
    void operator()(Trace<json::MatchOrders> const &) override { FAIL(); }
    void operator()(Trace<json::Orders> const &) override { FAIL(); }

    bool found = false;
  } handler;
  auto res = json::Parser2::dispatch(handler, message, buffers, {}, false);
  CHECK(res == true);
  CHECK(handler.found == true);
}
