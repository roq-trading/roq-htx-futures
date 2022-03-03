/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/trade.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_trade_simple_inverse", "json_trade") {
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
  CHECK(obj.ch == "market.BTC220325.trade.detail"sv);
  CHECK(obj.ts == 1639629424053ms);
  auto &tick = obj.tick;
  CHECK(tick.id == 150302535330);
  CHECK(tick.ts == 1639629424028ms);
  auto &data = tick.data;
  CHECK(std::size(data) == 1);
  auto &d0 = data[0];
  CHECK(d0.amount == 18);
  CHECK(d0.quantity == 0.0360801846022600627634855703315047361_a);
  CHECK(d0.ts == 1639629424028ms);
  CHECK(d0.id == 1503025353300000);
  CHECK(d0.price == 49888.88_a);
  CHECK(d0.direction == json::Side::BUY);
}

TEST_CASE("json_trade_simple_linear", "json_trade") {
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
  CHECK(obj.ch == "market.BTC-USDT.trade.detail"sv);
  CHECK(obj.ts == 1640775632524ms);
  auto &tick = obj.tick;
  CHECK(tick.id == 89747117157);
  CHECK(tick.ts == 1640775632497ms);
  auto &data = tick.data;
  CHECK(std::size(data) == 2);
  auto &d0 = data[0];
  CHECK(d0.amount == 120.0_a);
  CHECK(d0.quantity == 0.12_a);
  CHECK(d0.trade_turnover == 5724.132_a);
  CHECK(d0.ts == 1640775632497ms);
  CHECK(d0.id == 897471171570000);
  CHECK(d0.price == 47701.1_a);
  CHECK(d0.direction == json::Side::SELL);
  auto &d1 = data[1];
  CHECK(d1.amount == 2.0_a);
  CHECK(d1.quantity == 0.002_a);
  CHECK(d1.trade_turnover == 95.4022_a);
  CHECK(d1.ts == 1640775632497ms);
  CHECK(d1.id == 897471171570001);
  CHECK(d1.price == 47701.1_a);
  CHECK(d1.direction == json::Side::SELL);
}
