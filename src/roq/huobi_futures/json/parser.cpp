/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/json/parser.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "roq/compat.h"

#include "roq/logging.h"

#include "roq/huobi_futures/json/frame.h"
#include "roq/huobi_futures/json/topic.h"
#include "roq/huobi_futures/json/utils.h"

using namespace std::literals;

namespace roq {
namespace huobi_futures {
namespace json {

bool Parser::dispatch(
    Parser::Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    const server::TraceInfo &trace_info) {
  auto frame = core::json::Parser::create<json::Frame>(message, buffer);
  if (!frame.ping.count()) {
    switch (frame.status) {
      case Status::UNDEFINED: {
        Topic topic{extract_topic(frame.ch)};
        switch (topic) {
          case Topic::BBO: {
            auto bbo = core::json::Parser::create<json::BBO>(message, buffer);
            server::create_trace_and_dispatch(handler, trace_info, bbo);
            return true;
          }
          case Topic::DEPTH: {
            auto depth = core::json::Parser::create<json::Depth>(message, buffer);
            server::create_trace_and_dispatch(handler, trace_info, depth);
            return true;
          }
          case Topic::TRADE: {
            auto trade = core::json::Parser::create<json::Trade>(message, buffer);
            server::create_trace_and_dispatch(handler, trace_info, trade);
            return true;
          }
          case Topic::DETAIL: {
            auto detail = core::json::Parser::create<json::Detail>(message, buffer);
            server::create_trace_and_dispatch(handler, trace_info, detail);
            return true;
          }
          default:
            break;
        }
        break;
      }
      case Status::UNKNOWN:
        break;
      case Status::OK:
        if (!std::empty(frame.subbed)) {
          Subbed subbed{
              .id = frame.id,
              .subbed = frame.subbed,
              .ts = frame.ts,
              .status = frame.status,
          };
          server::create_trace_and_dispatch(handler, trace_info, subbed);
          return true;
        } else {
          log::fatal("DEBUG {}"sv, message);  // ???
        }
        break;
      case Status::ERROR:
        Error error{
            .id = frame.id,
            .err_code = frame.err_code,
            .err_msg = frame.err_msg,
            .ts = frame.ts,
        };
        server::create_trace_and_dispatch(handler, trace_info, error);
        return true;
    }
  } else {
    Ping ping{
        .timestamp = frame.ping,
    };
    server::create_trace_and_dispatch(handler, trace_info, ping);
    return true;
  }
  log::warn(R"(Unexpected: message="{}")"sv, message);
  return false;
}

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
