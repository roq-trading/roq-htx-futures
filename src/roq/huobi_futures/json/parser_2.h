/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/buffer.h"

#include "roq/server.h"

#include "roq/huobi_futures/json/close.h"
#include "roq/huobi_futures/json/funding_rate.h"
#include "roq/huobi_futures/json/ping.h"

namespace roq {
namespace huobi_futures {
namespace json {

struct Parser2 final {
  struct Handler {
    virtual void operator()(const server::Trace<Ping> &) = 0;
    virtual void operator()(const server::Trace<Close> &) = 0;
    virtual void operator()(const server::Trace<FundingRate> &) = 0;
  };

  static bool dispatch(
      Handler &,
      const std::string_view &message,
      core::json::Buffer &buffer,
      const server::TraceInfo &);
};

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
