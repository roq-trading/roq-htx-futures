/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/json/parser_2.hpp"

#include "roq/utils/compare.hpp"

#include "roq/logging.hpp"

#include "roq/htx_futures/json/frame_2.hpp"
#include "roq/htx_futures/json/topic_2.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace json {

// === HELPERS ===

namespace {
template <typename T>
void dispatch_helper(auto &handler, auto &message, auto &buffer_stack, auto &trace_info) {
  T obj{message, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj);
}

auto extract_topic(std::string_view const &topic) {
  auto separator = topic.find_last_of('.');
  auto name = topic.substr(separator == topic.npos ? 0 : (separator + 1));
  return Topic2{name};
}
}  // namespace

// === IMPLEMENTATION ===

bool Parser2::dispatch(
    Parser2::Handler &handler,
    std::string_view const &message,
    core::json::BufferStack &buffer_stack,
    TraceInfo const &trace_info,
    bool allow_unknown_event_types) {
  Frame2 frame{message, buffer_stack};
  switch (frame.op) {
    using enum Operator::type_t;
    case UNDEFINED_INTERNAL:
      break;
    case UNKNOWN_INTERNAL:
      if (allow_unknown_event_types) {
        return false;
      }
      break;
    case PING: {
      auto ping = Ping{
          .timestamp = frame.ts,
      };
      create_trace_and_dispatch(handler, trace_info, ping);
      return true;
    }
    case CLOSE: {
      auto close = Close{
          .timestamp = frame.ts,
      };
      create_trace_and_dispatch(handler, trace_info, close);
      return true;
    }
    case SUB:
      // drop
      return true;
    case NOTIFY: {
      auto topic = extract_topic(frame.topic);
      switch (topic) {
        using enum Topic2::type_t;
        case UNDEFINED_INTERNAL:
          break;
        case UNKNOWN_INTERNAL:
          if (allow_unknown_event_types) {
            return false;
          }
          break;
        case FUNDING_RATE:
          dispatch_helper<FundingRate>(handler, message, buffer_stack, trace_info);
          return true;
      }
      break;
    }
  }
  log::fatal(R"(Unexpected: message="{}")"sv, message);
}

}  // namespace json
}  // namespace htx_futures
}  // namespace roq
