/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/trade.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

TEST(json_trade, simple) {
  auto message = R"({)"
                 R"("ch":"market.BTC220325.trade.detail",)"
                 R"("ts":1639629424053,)"
                 R"("tick":{)"
                 R"("id":150302535330,)"
                 R"("ts":1639629424028,)"
                 R"("data":[{)"
                 R"("amount":18,)"
                 R"("quantity":0.0360801846022600627634855703315047361,)"
                 R"("ts":1639629424028,)"
                 R"("id":1503025353300000,)"
                 R"("price":49888.88,)"
                 R"("direction":"buy")"
                 R"(})"
                 R"(])"
                 R"(})"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::Trade>(message, buffer_);
}
