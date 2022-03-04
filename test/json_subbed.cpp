/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/subbed.h"

using namespace roq;
using namespace roq::huobi_futures;

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
  // auto obj = core::json::Parser::create<json::Subbed>(message, buffer_);
  */
}
