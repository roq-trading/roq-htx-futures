/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/json/parser.hpp"

#include "roq/huobi_futures/json/utils.hpp"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_utils_extract_symbol", "[json_utils]") {
  CHECK(json::extract_symbol("market.TRX211224.bbo"sv) == "TRX211224"sv);
  CHECK(json::extract_symbol("market.FIL211231.trade.detail"sv) == "FIL211231"sv);
  CHECK(json::extract_symbol("market.FIL211231.detail"sv) == "FIL211231"sv);
  CHECK(json::extract_symbol("market.FIL211231.depth.size_150.high_freq"sv) == "FIL211231"sv);
  CHECK(json::extract_symbol("market.BTC-USD.estimated_rate.60min"sv) == "BTC-USD"sv);
  CHECK(json::extract_symbol("market.BTC-USD.premium_index.1min"sv) == "BTC-USD"sv);
}

TEST_CASE("json_utils_extract_topic", "[json_utils]") {
  CHECK(json::extract_topic("market.TRX211224.bbo"sv) == "bbo"sv);
  CHECK(json::extract_topic("market.FIL211231.trade.detail"sv) == "trade"sv);
  CHECK(json::extract_topic("market.FIL211231.detail"sv) == "detail"sv);
  CHECK(json::extract_topic("market.FIL211231.depth.size_150.high_freq"sv) == "depth"sv);
  CHECK(json::extract_topic("market.BTC-USD.estimated_rate.60min"sv) == "estimated_rate"sv);
  CHECK(json::extract_topic("market.BTC-USD.premium_index.1min"sv) == "premium_index"sv);
}
