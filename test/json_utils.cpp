/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/utils.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

// note! reduced
TEST(json_utils, extract_symbol) {
  auto symbol = json::extract_symbol("market.TRX211224.bbo"sv);
  EXPECT_EQ(symbol, "TRX211224"sv);
}
