/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::PremiumIndex;

TEST_CASE("json_premium_index_simple_swap", "[json_premium_index]") {
  auto message = R"({)"
                 R"("ch":"market.BTC-USD.premium_index.1min",)"
                 R"("ts":1642657680747,)"
                 R"("tick":{)"
                 R"("id":1642657680,)"
                 R"("open":"-0.0002871886442248",)"
                 R"("close":"-0.0002871886442248",)"
                 R"("high":"-0.0002871886442248",)"
                 R"("low":"-0.0002871886442248",)"
                 R"("amount":"0",)"
                 R"("vol":"0",)"
                 R"("count":"0")"
                 R"(})"
                 R"(})";
  auto helper = [](value_type const &obj) {
    CHECK(obj.ch == "market.BTC-USD.premium_index.1min"sv);
    CHECK(obj.ts == 1642657680747ms);
    auto &tick = obj.tick;
    CHECK(tick.id == 1642657680);
    CHECK(tick.id == 1642657680);
    CHECK(tick.open == -0.0002871886442248_a);
    CHECK(tick.close == -0.0002871886442248_a);
    CHECK(tick.high == -0.0002871886442248_a);
    CHECK(tick.low == -0.0002871886442248_a);
    CHECK(tick.amount == 0.0_a);
    CHECK(tick.vol == 0.0_a);
    CHECK(tick.count == 0.0_a);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
