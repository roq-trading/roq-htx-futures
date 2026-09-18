/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/protocol/json/place_order_ack5.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::PlaceOrderAck5;

TEST_CASE("failure", "[json_place_order_ack_5]") {
  auto message = R"({)"
                 R"("code":1067,)"
                 R"("message":"Illegal parameter margin_mode.",)"
                 R"("data":null,)"
                 R"("args":null,)"
                 R"("ts":1789707701128,)"
                 R"("unknownException":false)"
                 R"(})";
  auto helper = [&](value_type &obj) { CHECK(obj.code == 1067); };
  value_type obj{message};
  helper(obj);
}
