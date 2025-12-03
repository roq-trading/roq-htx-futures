/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::EstimatedRate;

TEST_CASE("swap", "[json_estimated_rate]") {
  auto message = R"({)"
                 R"("ch":"market.BTC-USD.estimated_rate.1min",)"
                 R"("ts":1642658280847,)"
                 R"("tick":{)"
                 R"("id":1642658280,)"
                 R"("open":"0.0001",)"
                 R"("close":"0.0001",)"
                 R"("high":"0.0001",)"
                 R"("low":"0.0001",)"
                 R"("amount":"0",)"
                 R"("vol":"0",)"
                 R"("count":"0")"
                 R"(})"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.ch == "market.BTC-USD.estimated_rate.1min"sv); };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
