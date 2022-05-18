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
auto extract_topic(std::string_view const &topic) {
  auto separator = topic.find_last_of('.');
  auto name = topic.substr(separator == topic.npos ? 0 : (separator + 1));
  return Topic2{name};
}
}  // namespace

bool Parser2::dispatch(
    Parser2::Handler &handler,
    std::string_view const &message,
    core::json::Buffer &buffer,
    TraceInfo const &trace_info) {
  auto frame = core::json::Parser::create<json::Frame2>(message, buffer);
  switch (frame.op) {
    using enum Operator::type_t;
    case UNDEFINED:
    case UNKNOWN:
      break;
    case PING: {
      const Ping ping{
          .timestamp = frame.ts,
      };
      create_trace_and_dispatch(handler, trace_info, ping);
      return true;
    }
    case CLOSE: {
      const Close close{
          .timestamp = frame.ts,
      };
      create_trace_and_dispatch(handler, trace_info, close);
      return true;
    }
    case SUB:
      return true;
    case NOTIFY: {
      auto topic = extract_topic(frame.topic);
      switch (topic) {
        using enum Topic2::type_t;
        case UNDEFINED:
          break;
        case UNKNOWN:
          break;
        case FUNDING_RATE: {
          auto const funding_rate = core::json::Parser::create<json::FundingRate>(message, buffer);
          create_trace_and_dispatch(handler, trace_info, funding_rate);
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
