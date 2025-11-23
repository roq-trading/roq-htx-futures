/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/error_2.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("simple", "[json_error]") {
  auto message = R"({)"
                 R"("op":"error",)"
                 R"("ts":1763860751249)"
                 R"(})";
  [[maybe_unused]] json::Error2 obj{message};
}
