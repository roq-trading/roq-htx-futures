/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/depth.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

// note! reduced
TEST(json_depth, simple) {
  auto message = R"({)"
                 R"("ch":"market.FIL211231.depth.size_150.high_freq",)"
                 R"("tick":{)"
                 R"("asks":[)"
                 R"([37.722,2],)"
                 R"([37.735,145])"
                 R"(],)"
                 R"("bids":[)"
                 R"([37.71,145],)"
                 R"([37.709,170])"
                 R"(],)"
                 R"("ch":"market.FIL211231.depth.size_150.high_freq",)"
                 R"("event":"snapshot",)"
                 R"("id":149496559186,)"
                 R"("mrid":149496559186,)"
                 R"("ts":1639630955318,)"
                 R"("version":195613528)"
                 R"(},)"
                 R"("ts":1639630955318)"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::Depth>(message, buffer_);
}
