/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_2_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::Sub;

TEST_CASE("simple", "[json_sub]") {
  auto message = R"({)"
                 R"("op":"sub",)"
                 R"("topic":"orders.*",)"
                 R"("ts":1763808315805,)"
                 R"("err-code":0)"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == protocol::json::Operator::SUB); };
  Parser2Tester<value_type>::dispatch(helper, message, 8192, 1);
}
