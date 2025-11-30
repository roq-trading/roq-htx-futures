/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/parser_2.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("simple", "[json_error]") {
  auto message = R"({)"
                 R"("op":"error",)"
                 R"("ts":1763860751249)"
                 R"(})";
  core::json::BufferStack buffers{8192, 1};
  // simple
  json::Error2 obj{message};
  CHECK(obj.op == json::Operator::ERROR);
  // parser
  struct Handler final : public json::Parser2::Handler {
    void operator()(Trace<json::Close> const &) override { FAIL(); }
    void operator()(Trace<json::Error2> const &event) override {
      found = true;
      auto &[trace_info, error] = event;
      CHECK(error.op == json::Operator::ERROR);
    }
    void operator()(Trace<json::Ping> const &) override { FAIL(); }
    void operator()(Trace<json::Auth> const &) override { FAIL(); }
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
