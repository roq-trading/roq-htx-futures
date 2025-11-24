/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/place_order_ack.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("error", "[json_place_order_ack]") {
  auto message = R"({)"
                 R"("status":"error",)"
                 R"("err_code":1047,)"
                 R"("err_msg":"Insufficient margin available.",)"
                 R"("ts":1763867554021)"
                 R"(})";
  [[maybe_unused]] json::PlaceOrderAck obj{message};
}

TEST_CASE("success", "[json_place_order_ack]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":{)"
                 R"("order_id":1442212759625592832,)"
                 R"("client_order_id":563231976427594,)"
                 R"("order_id_str":"1442212759625592832")"
                 R"(},)"
                 R"("ts":1763891916921)"
                 R"(})";
  [[maybe_unused]] json::PlaceOrderAck obj{message};
}
