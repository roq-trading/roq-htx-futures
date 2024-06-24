/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/huobi_futures/json/estimated_rate.hpp"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_estimated_rate_simple_swap", "[json_estimated_rate]") {
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
  std::vector<std::byte> buffer(8192);
  [[maybe_unused]] json::EstimatedRate obj{message, buffer};
}
