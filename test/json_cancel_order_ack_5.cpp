/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/protocol/json/cancel_order_ack5.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::CancelOrderAck5;

TEST_CASE("success", "[json_cancel_order_ack_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":{)"
                 R"("order_id":"1550497891998535681",)"
                 R"("client_order_id":"577024242451838431")"
                 R"(},)"
                 R"("args":null,)"
                 R"("ts":1789716092325,)"
                 R"("unknownException":false)"
                 R"(})";
  auto helper = [&](value_type &obj) { CHECK(obj.code == 200); };
  value_type obj{message};
  helper(obj);
}
