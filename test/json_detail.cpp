/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/detail.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

TEST(json_detail, simple_inverse) {
  auto message = R"({)"
                 R"("ch":"market.FIL211231.detail",)"
                 R"("ts":1639628009780,)"
                 R"("tick":{)"
                 R"("id":1639627980,)"
                 R"("mrid":149495401281,)"
                 R"("open":35.714,)"
                 R"("close":37.795,)"
                 R"("high":38.901,)"
                 R"("low":35.641,)"
                 R"("amount":68725.1520427579159023020649636609479771038,)"
                 R"("vol":255300,)"
                 R"("count":4172,)"
                 R"("ask":[37.809,145],)"
                 R"("bid":[37.783,4])"
                 R"(})"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::Detail>(message, buffer_);
}

TEST(json_detail, simple_linear) {
  auto message = R"({)"
                 R"("ch":"market.WOO-USDT.detail",)"
                 R"("ts":1640775846213,)"
                 R"("tick":{)"
                 R"("id":1640775840,)"
                 R"("mrid":38292289192,)"
                 R"("open":0.99934,)"
                 R"("close":0.89934,)"
                 R"("high":1.00166,)"
                 R"("low":0.88366,)"
                 R"("amount":738940,)"
                 R"("vol":73894,)"
                 R"("trade_turnover":683194.497,)"
                 R"("count":4256,)"
                 R"("ask":[0.90157,10],)"
                 R"("bid":[0.89738,235])"
                 R"(})"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::Detail>(message, buffer_);
}
