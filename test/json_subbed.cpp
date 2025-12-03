/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::Subbed;

// note! just a struct
TEST_CASE("simple", "[json_subbed]") {
  /*
  auto message = R"({)"
                 R"("id":"3000001",)"
                 R"("subbed":"market.FIL211231.bbo",)"
                 R"("ts":1639584082288,)"
                 R"("status":"ok")"
                 R"(})";
  auto helper = [](value_type const &obj) {
    CHECK(obj.id==3000001);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
  */
}
