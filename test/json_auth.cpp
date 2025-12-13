/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_2_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::Auth;

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
  auto helper = [](value_type const &obj) { CHECK(obj.op == json::Operator::AUTH); };
  Parser2Tester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("failure", "[json_auth]") {
  auto message = R"({)"
                 R"("op":"auth",)"
                 R"("type":"api",)"
                 R"("ts":1763810619267,)"
                 R"("err-code":2003,)"
                 R"("err-msg":"Verification failure [校验失败]")"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == json::Operator::AUTH); };
  Parser2Tester<value_type>::dispatch(helper, message, 8192, 1);
}
