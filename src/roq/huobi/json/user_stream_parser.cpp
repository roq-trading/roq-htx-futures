/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi/json/user_stream_parser.h"

#include "roq/logging.h"

#include "roq/core/json/parser.h"

using namespace roq::literals;

namespace roq {
namespace huobi {
namespace json {

void UserStreamParser::dispatch(
    UserStreamParser::Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    const server::TraceInfo &trace_info) {
  core::json::Parser parser(message);
  auto root = parser.root();
  for (auto [key, value] : std::get<core::json::object_t>(root)) {
    if (key.compare("e"_sv) != 0)
      continue;
    EventType event_type(value);
    if (try_dispatch(handler, message, buffer, event_type, trace_info))
      return;
    break;
  }
  log::warn(R"(message="{}")"_fmt, message);
  log::fatal("Unexpected"_sv);
}

bool UserStreamParser::try_dispatch(
    UserStreamParser::Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    EventType event_type,
    const server::TraceInfo &trace_info) {
  switch (event_type) {
    case EventType::UNDEFINED:
    case EventType::UNKNOWN:
    case EventType::AGG_TRADE:
    case EventType::TRADE:
    case EventType::_24HR_MINI_TICKER:
    case EventType::BOOK_TICKER:
    case EventType::DEPTH_UPDATE:
      log::fatal("Unexpected"_sv);
      break;
    case EventType::OUTBOUND_ACCOUNT_INFO: {
      auto outbound_account_info = core::json::Parser::create<OutboundAccountInfo>(message, buffer);
      handler(outbound_account_info, trace_info);
      break;
    }
    case EventType::OUTBOUND_ACCOUNT_POSITION: {
      auto outbound_account_position =
          core::json::Parser::create<OutboundAccountPosition>(message, buffer);
      handler(outbound_account_position, trace_info);
      break;
    }
    case EventType::BALANCE_UPDATE: {
      auto balance_update = core::json::Parser::create<BalanceUpdate>(message);
      handler(balance_update, trace_info);
      break;
    }
    case EventType::EXECUTION_REPORT: {
      auto execution_report = core::json::Parser::create<ExecutionReport>(message);
      handler(execution_report, trace_info);
      break;
    }
    case EventType::LIST_STATUS:
      return false;  // XXX implement this
    default:
      return false;
  }
  return true;
}

}  // namespace json
}  // namespace huobi
}  // namespace roq
