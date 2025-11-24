/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/cancel_all_orders_ack.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("error", "[json_cancel_all_orders_ack]") {
  auto message = R"({)"
                 R"("status":"error",)"
                 R"("err_code":403,)"
                 R"("err_msg":"Incorrect signature method [错误的签名方法]",)"
                 R"("ts":1763892684776)"
                 R"(})";
  [[maybe_unused]] json::CancelAllOrdersAck obj{message};
}

TEST_CASE("success", "[json_cancel_all_orders_ack]") {
  auto message = R"({)"
                 R"("status":"ok",)"
                 R"("data":{)"
                 R"("errors":[],)"
                 R"("successes":"1442564726236282880")"
                 R"(},)"
                 R"("ts":1763975842675)"
                 R"(})";
  [[maybe_unused]] json::CancelAllOrdersAck obj{message};
}
