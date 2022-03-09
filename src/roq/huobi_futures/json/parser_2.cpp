/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/json/parser_2.hpp"

#include "roq/utils/compare.hpp"

#include "roq/logging.hpp"

#include "roq/huobi_futures/json/frame_2.hpp"
#include "roq/huobi_futures/json/topic_2.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {
namespace json {

namespace {
auto extract_topic(const std::string_view &topic) {
  auto separator = topic.find_last_of('.');
  auto name = topic.substr(separator == topic.npos ? 0 : (separator + 1));
  return Topic2{name};
}
}  // namespace

bool Parser2::dispatch(
    Parser2::Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    const server::TraceInfo &trace_info) {
  auto frame = core::json::Parser::create<json::Frame2>(message, buffer);
  switch (frame.op) {
    case Operator::UNDEFINED:
    case Operator::UNKNOWN:
      break;
    case Operator::PING: {
      Ping ping{
          .timestamp = frame.ts,
      };
      server::create_trace_and_dispatch(handler, trace_info, ping);
      return true;
    }
    case Operator::CLOSE: {
      Close close{
          .timestamp = frame.ts,
      };
      server::create_trace_and_dispatch(handler, trace_info, close);
      return true;
    }
    case Operator::SUB:
      return true;
    case Operator::NOTIFY: {
      auto topic = extract_topic(frame.topic);
      switch (topic) {
        case Topic2::UNDEFINED:
          break;
        case Topic2::UNKNOWN:
          break;
        case Topic2::FUNDING_RATE: {
          auto funding_rate = core::json::Parser::create<json::FundingRate>(message, buffer);
          server::create_trace_and_dispatch(handler, trace_info, funding_rate);
          return true;
        }
      }
      break;
    }
  }
  log::warn(R"(Unexpected: message="{}")"sv, message);
  return false;
}

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
