/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/trade.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

TEST(json_trade, simple_inverse) {
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

TEST(json_trade, simple_linear) {
  auto message = R"({)"
                 R"("ch":"market.BTC-USDT.trade.detail",)"
                 R"("ts":1640775632524,)"
                 R"("tick":{)"
                 R"("id":89747117157,)"
                 R"("ts":1640775632497,)"
                 R"("data":[{)"
                 R"("amount":120,)"
                 R"("quantity":0.12,)"
                 R"("trade_turnover":5724.132,)"
                 R"("ts":1640775632497,)"
                 R"("id":897471171570000,)"
                 R"("price":47701.1,)"
                 R"("direction":"sell")"
                 R"(},{)"
                 R"("amount":2,)"
                 R"("quantity":0.002,)"
                 R"("trade_turnover":95.4022,)"
                 R"("ts":1640775632497,)"
                 R"("id":897471171570001,)"
                 R"("price":47701.1,)"
                 R"("direction":"sell")"
                 R"(})"
                 R"(])"
                 R"(})"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::Trade>(message, buffer_);
}
