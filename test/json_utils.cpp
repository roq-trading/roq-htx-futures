/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/json/parser.h"

#include "roq/huobi_futures/json/utils.h"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

TEST(json_utils, extract_symbol) {
  EXPECT_EQ(json::extract_symbol("market.TRX211224.bbo"sv), "TRX211224"sv);
  EXPECT_EQ(json::extract_symbol("market.FIL211231.trade.detail"sv), "FIL211231"sv);
  EXPECT_EQ(json::extract_symbol("market.FIL211231.detail"sv), "FIL211231"sv);
  EXPECT_EQ(json::extract_symbol("market.FIL211231.depth.size_150.high_freq"sv), "FIL211231"sv);
  EXPECT_EQ(json::extract_symbol("market.BTC-USD.estimated_rate.60min"sv), "BTC-USD"sv);
  EXPECT_EQ(json::extract_symbol("market.BTC-USD.premium_index.1min"sv), "BTC-USD"sv);
}

TEST(json_utils, extract_topic) {
  EXPECT_EQ(json::extract_topic("market.TRX211224.bbo"sv), "bbo"sv);
  EXPECT_EQ(json::extract_topic("market.FIL211231.trade.detail"sv), "trade"sv);
  EXPECT_EQ(json::extract_topic("market.FIL211231.detail"sv), "detail"sv);
  EXPECT_EQ(json::extract_topic("market.FIL211231.depth.size_150.high_freq"sv), "depth"sv);
  EXPECT_EQ(json::extract_topic("market.BTC-USD.estimated_rate.60min"sv), "estimated_rate"sv);
  EXPECT_EQ(json::extract_topic("market.BTC-USD.premium_index.1min"sv), "premium_index"sv);
}
