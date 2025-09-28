/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/htx_futures/json/subbed.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_subbed_simple", "[json_subbed]") {
  /*
  auto message = R"({)"
                 R"("id":"3000001",)"
                 R"("subbed":"market.FIL211231.bbo",)"
                 R"("ts":1639584082288,)"
                 R"("status":"ok")"
                 R"(})";
  json::Subbed obj{message, buffer_};
  */
}
