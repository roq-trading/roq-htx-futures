/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/htx_futures/protocol/json/response5.hpp"
#include "roq/htx_futures/protocol/json/response5_multiple.hpp"
#include "roq/htx_futures/protocol/json/response5_single.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = protocol::json::Response5;

TEST_CASE("create_order_ack", "[json_response_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":{)"
                 R"("order_id":"1550915326489534464",)"
                 R"("client_order_id":"577024243446975415")"
                 R"(},)"
                 R"("args":null,)"
                 R"("ts":1789808628460,)"
                 R"("unknownException":false,)"
                 R"("cid":"P:577024243446975415:1",)"
                 R"("rate_limit":{)"
                 R"("limit":"72",)"
                 R"("interval":"3000",)"
                 R"("remaining":"71",)"
                 R"("reset":"1789808631448")"
                 R"(})"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200);
    REQUIRE(!std::empty(obj.data));
    REQUIRE(obj.data[0] == '{');
    protocol::json::Response5Single data{obj.data};
    CHECK(data.order_id == "1550915326489534464"sv);
    CHECK(data.client_order_id == "577024243446975415"sv);
  };
  value_type obj{message};
  helper(obj);
}

TEST_CASE("create_order_ack_failure", "[json_response_5]") {
  auto message = R"({)"
                 R"("code":1497,)"
                 R"("message":"Position mode parameter passing error!",)"
                 R"("data":null,)"
                 R"("args":null,)"
                 R"("ts":1789900759774,)"
                 R"("unknownException":false,)"
                 R"("cid":"P:577024244367613219:1",)"
                 R"("rate_limit":{)"
                 R"("limit":"72",)"
                 R"("interval":"3000",)"
                 R"("remaining":"71",)"
                 R"("reset":"1789900762749")"
                 R"(})"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 1497);
    REQUIRE(std::empty(obj.data));
  };
  value_type obj{message};
  helper(obj);
}

TEST_CASE("cancel_order_ack", "[json_response_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":{)"
                 R"("order_id":"1550915326489534464",)"
                 R"("client_order_id":"577024243446975415")"
                 R"(},)"
                 R"("args":null,)"
                 R"("ts":1789808702398,)"
                 R"("unknownException":false,)"
                 R"("cid":"C:577024243446975415:2",)"
                 R"("rate_limit":{)"
                 R"("limit":"72",)"
                 R"("interval":"3000",)"
                 R"("remaining":"71",)"
                 R"("reset":"1789808705397")"
                 R"(})"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200);
    REQUIRE(!std::empty(obj.data));
    REQUIRE(obj.data[0] == '{');
    protocol::json::Response5Single data{obj.data};
    CHECK(data.order_id == "1550915326489534464"sv);
    CHECK(data.client_order_id == "577024243446975415"sv);
  };
  value_type obj{message};
  helper(obj);
}

TEST_CASE("cancel_all_orders_ack_1", "[json_response_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":[{)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("order_id":"1550913854274416640",)"
                 R"("client_order_id":"577024243443546885")"
                 R"(})"
                 R"(],)"
                 R"("args":null,)"
                 R"("ts":1789808279522,)"
                 R"("unknownException":false,)"
                 R"("cid":"X:577023702256844800:0",)"
                 R"("rate_limit":{)"
                 R"("limit":"72",)"
                 R"("interval":"3000",)"
                 R"("remaining":"70",)"
                 R"("reset":"1789808280443")"
                 R"(})"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200);
    REQUIRE(!std::empty(obj.data));
    REQUIRE(obj.data[0] == '[');
    core::json::BufferStack buffers{8192, 1};
    protocol::json::Response5Multiple data{obj.data, buffers};
    REQUIRE(std::size(data.data) == 1);
    auto &d0 = data.data[0];
    CHECK(d0.code == 200);
    CHECK(d0.message == "Success"sv);
    CHECK(d0.order_id == "1550913854274416640"sv);
    CHECK(d0.client_order_id == "577024243443546885"sv);
  };
  value_type obj{message};
  helper(obj);
}

TEST_CASE("cancel_all_orders_ack_2", "[json_response_5]") {
  auto message = R"({)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("data":[{)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("order_id":"1550926886108626945",)"
                 R"("client_order_id":"577024243474555008")"
                 R"(},{)"
                 R"("code":200,)"
                 R"("message":"Success",)"
                 R"("order_id":"1550926861005717504",)"
                 R"("client_order_id":"577024243474555007")"
                 R"(})"
                 R"(],)"
                 R"("args":null,)"
                 R"("ts":1789811392330,)"
                 R"("unknownException":false,)"
                 R"("cid":"X:577023702256844800:0",)"
                 R"("rate_limit":{)"
                 R"("limit":"72",)"
                 R"("interval":"3000",)"
                 R"("remaining":"71",)"
                 R"("reset":"1789811395329")"
                 R"(})"
                 R"(})";
  auto helper = [&](value_type &obj) {
    CHECK(obj.code == 200);
    REQUIRE(!std::empty(obj.data));
    REQUIRE(obj.data[0] == '[');
    core::json::BufferStack buffers{8192, 1};
    protocol::json::Response5Multiple data{obj.data, buffers};
    REQUIRE(std::size(data.data) == 2);
    auto &d0 = data.data[0];
    CHECK(d0.code == 200);
    CHECK(d0.message == "Success"sv);
    CHECK(d0.order_id == "1550926886108626945"sv);
    CHECK(d0.client_order_id == "577024243474555008"sv);
    auto &d1 = data.data[1];
    CHECK(d1.code == 200);
    CHECK(d1.message == "Success"sv);
    CHECK(d1.order_id == "1550926861005717504"sv);
    CHECK(d1.client_order_id == "577024243474555007"sv);
  };
  value_type obj{message};
  helper(obj);
}
