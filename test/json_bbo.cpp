/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::BBO;

TEST_CASE("simple", "[json_bbo]") {
  auto message = R"({)"
                 R"("ch":"market.TRX211224.bbo",)"
                 R"("ts":1639583658324,)"
                 R"("tick":{)"
                 R"("mrid":131671795982,)"
                 R"("id":1639583658,)"
                 R"("bid":[0.08308,71],)"
                 R"("ask":[0.08337,247],)"
                 R"("ts":1639583658324,)"
                 R"("version":131671795982,)"
                 R"("ch":"market.TRX211224.bbo")"
                 R"(})"
                 R"(})";
  auto helper = [](value_type const &obj) {
    CHECK(obj.ch == "market.TRX211224.bbo"sv);
    CHECK(obj.ts == 1639583658324ms);
    auto &tick = obj.tick;
    CHECK(tick.mrid == 131671795982);
    CHECK(tick.id == 1639583658);
    CHECK(tick.bid.price == 0.08308_a);
    CHECK(tick.bid.vol == 71.0_a);
    CHECK(tick.ask.price == 0.08337_a);
    CHECK(tick.ask.vol == 247.0_a);
    CHECK(tick.ts == 1639583658324ms);
    CHECK(tick.version == 131671795982);
    CHECK(tick.ch == "market.TRX211224.bbo"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
