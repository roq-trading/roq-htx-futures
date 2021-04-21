/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/buffer.h"

#include "roq/server.h"

#include "roq/huobi_futures/json/event_type.h"

#include "roq/huobi_futures/json/balance_update.h"
#include "roq/huobi_futures/json/execution_report.h"
#include "roq/huobi_futures/json/outbound_account_info.h"
#include "roq/huobi_futures/json/outbound_account_position.h"

namespace roq {
namespace huobi_futures {
namespace json {

struct UserStreamParser final {
  struct Handler {
    virtual void operator()(const OutboundAccountInfo &, const server::TraceInfo &) = 0;
    virtual void operator()(const OutboundAccountPosition &, const server::TraceInfo &) = 0;
    virtual void operator()(const BalanceUpdate &, const server::TraceInfo &) = 0;
    virtual void operator()(const ExecutionReport &, const server::TraceInfo &) = 0;
  };

  static void dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      const server::TraceInfo &trace);

 private:
  static bool try_dispatch(
      UserStreamParser::Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      EventType event_type,
      const server::TraceInfo &trace);
};

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
