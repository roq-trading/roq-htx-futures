/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/sub.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("simple", "[json_sub]") {
  auto message = R"({)"
                 R"("op":"sub",)"
                 R"("topic":"orders.*",)"
                 R"("ts":1763808315805,)"
                 R"("err-code":0)"
                 R"(})";
  [[maybe_unused]] json::Sub obj{message};
}
