/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/estimated_rate.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

TEST(json_estimated_rate, simple_swap) {
  auto message = R"({)"
                 R"("ch":"market.BTC-USD.estimated_rate.1min",)"
                 R"("ts":1642658280847,)"
                 R"("tick":{)"
                 R"("id":1642658280,)"
                 R"("open":"0.0001",)"
                 R"("close":"0.0001",)"
                 R"("high":"0.0001",)"
                 R"("low":"0.0001",)"
                 R"("amount":"0",)"
                 R"("vol":"0",)"
                 R"("count":"0")"
                 R"(})"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::EstimatedRate>(message, buffer_);
}
