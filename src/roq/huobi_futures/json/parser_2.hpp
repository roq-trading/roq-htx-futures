/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/huobi_futures/json/close.hpp"
#include "roq/huobi_futures/json/funding_rate.hpp"
#include "roq/huobi_futures/json/ping.hpp"

namespace roq {
namespace huobi_futures {
namespace json {

struct Parser2 final {
  struct Handler {
    virtual void operator()(Trace<Ping> const &) = 0;
    virtual void operator()(Trace<Close> const &) = 0;
    virtual void operator()(Trace<FundingRate> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, std::span<std::byte> const &, TraceInfo const &);
};

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
