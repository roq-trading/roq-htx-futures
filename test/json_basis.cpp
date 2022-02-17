/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/basis.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

TEST(json_basis, simple_swap) {
  auto message = R"({)"
                 R"("ch":"market.WOO-USDT.basis.1min.open",)"
                 R"("ts":1642659617542,)"
                 R"("tick":{)"
                 R"("id":1642659600,)"
                 R"("index_price":"0.8645621602666667",)"
                 R"("contract_price":"0.851",)"
                 R"("basis":"-0.0135621602666667",)"
                 R"("basis_rate":"-0.0156867382010838518425939816818336619"})"
                 R"(})";
  core::Buffer buffer(8192);
  core::json::Buffer buffer_(buffer);
  auto obj = core::json::Parser::create<json::Basis>(message, buffer_);
  EXPECT_EQ(obj.ch, "market.WOO-USDT.basis.1min.open"sv);
  EXPECT_EQ(obj.ts, 1642659617542ms);
  auto &tick = obj.tick;
  EXPECT_EQ(tick.id, 1642659600);
  EXPECT_DOUBLE_EQ(tick.index_price, 0.8645621602666667);
  EXPECT_DOUBLE_EQ(tick.contract_price, 0.851);
  EXPECT_DOUBLE_EQ(tick.basis, -0.0135621602666667);
  EXPECT_DOUBLE_EQ(tick.basis_rate, -0.0156867382010838518425939816818336619);
}
