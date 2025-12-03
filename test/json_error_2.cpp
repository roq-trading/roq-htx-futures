/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_2_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::Error2;

TEST_CASE("simple", "[json_error]") {
  auto message = R"({)"
                 R"("op":"error",)"
                 R"("ts":1763860751249)"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == json::Operator::ERROR); };
  Parser2Tester<value_type>::dispatch(helper, message, 8192, 1);
}
