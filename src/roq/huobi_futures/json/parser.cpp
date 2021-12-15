/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/json/parser.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "roq/compat.h"

#include "roq/logging.h"

#include "roq/huobi_futures/json/bbo_frame.h"
#include "roq/huobi_futures/json/frame.h"

using namespace std::literals;

namespace roq {
namespace huobi_futures {
namespace json {

namespace {
std::string_view get_channel(const std::string_view &channel) {
  auto offset = channel.find('.');
  return channel.substr(0, offset);
}
}  // namespace

bool Parser::dispatch(
    Parser::Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    const server::TraceInfo &trace_info) {
  auto frame = core::json::Parser::create<json::Frame>(message, buffer);
  if (!frame.ping.count()) {
    switch (frame.status) {
      case Status::UNDEFINED: {
        auto channel = get_channel(frame.ch);
        if (channel.compare("market"sv) == 0) {
          auto bbo_frame = core::json::Parser::create<json::BBOFrame>(message, buffer);
          server::create_trace_and_dispatch(handler, trace_info, bbo_frame.tick);
          return true;
        }
        break;
      }
      case Status::UNKNOWN:
        log::warn(R"(Unexpected: message="{}")"sv, message);
        return false;
      case Status::OK:
        log::debug("OK"sv);
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
  return false;
}

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
