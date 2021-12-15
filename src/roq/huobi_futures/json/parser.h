/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/buffer.h"

#include "roq/server.h"

#include "roq/huobi_futures/json/bbo.h"
#include "roq/huobi_futures/json/error.h"
#include "roq/huobi_futures/json/ping.h"
#include "roq/huobi_futures/json/subbed.h"

namespace roq {
namespace huobi_futures {
namespace json {

struct Parser final {
  struct Handler {
    virtual void operator()(const server::Trace<Ping> &) = 0;
    virtual void operator()(const server::Trace<Error> &) = 0;
    virtual void operator()(const server::Trace<Subbed> &) = 0;
    virtual void operator()(const server::Trace<BBO> &) = 0;
  };

  static bool dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      const server::TraceInfo &);
};

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
