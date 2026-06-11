/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_3_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::Error2;

TEST_CASE("simple", "[json_error_3]") {
  auto message = R"({)"
                 R"("op":"error",)"
                 R"("ts":1763860751249)"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == protocol::json::Operator::ERROR); };
  Parser3Tester<value_type>::dispatch(helper, message, 8192, 1);
}
