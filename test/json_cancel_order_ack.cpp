/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/cancel_order_ack.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::CancelOrderAck;

TEST_CASE("success", "[json_cancel_order_ack]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":{)"
                 R"("errors":[],)"
                 R"("successes":"563232668624008")"
                 R"(},)"
                 R"("ts":1763961141034)"
                 R"(})";
  auto helper = [&](value_type &obj) { CHECK(obj.status == json::Status::OK); };
  value_type obj{message};
  helper(obj);
}
