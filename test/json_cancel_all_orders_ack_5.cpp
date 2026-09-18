/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/protocol/json/cancel_all_orders_ack5.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::CancelAllOrdersAck5;

TEST_CASE("success", "[json_cancel_all_orders_ack_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":[{)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("order_id":"1550600299097096193",)"
                 R"("client_order_id":"577024242695969075")"
                 R"(})"
                 R"(],)"
                 R"("args":null,)"
                 R"("ts":1789733522623,)"
                 R"("unknownException":false)"
                 R"(})";
  auto helper = [&](value_type &obj) { CHECK(obj.code == 200); };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}
