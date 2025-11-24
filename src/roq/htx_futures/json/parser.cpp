/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/json/parser.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "roq/compat.hpp"

#include "roq/logging.hpp"

#include "roq/htx_futures/json/frame.hpp"
#include "roq/htx_futures/json/topic.hpp"
#include "roq/htx_futures/json/utils.hpp"

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
}  // namespace

// === IMPLEMENTATION ===

bool Parser::dispatch(
    Parser::Handler &handler,
    std::string_view const &message,
    core::json::BufferStack &buffer_stack,
    TraceInfo const &trace_info,
    bool allow_unknown_event_types) {
  Frame frame{message, buffer_stack};
  if (!frame.ping.count()) {
    switch (frame.status) {
      using enum Status::type_t;
      case UNDEFINED_INTERNAL: {
        Topic topic{extract_topic(frame.ch)};
        switch (topic) {
          using enum Topic::type_t;
          case UNDEFINED_INTERNAL:
            break;
          case UNKNOWN_INTERNAL:
            if (allow_unknown_event_types) {
              return false;
            }
            break;
          case BBO:
            dispatch_helper<json::BBO>(handler, message, buffer_stack, trace_info);
            return true;
          case DEPTH:
            dispatch_helper<Depth>(handler, message, buffer_stack, trace_info);
            return true;
          case TRADE:
            dispatch_helper<Trade>(handler, message, buffer_stack, trace_info);
            return true;
          case DETAIL:
            dispatch_helper<Detail>(handler, message, buffer_stack, trace_info);
            return true;
          case ESTIMATED_RATE:
            dispatch_helper<EstimatedRate>(handler, message, buffer_stack, trace_info);
            return true;
          case PREMIUM_INDEX:
            dispatch_helper<PremiumIndex>(handler, message, buffer_stack, trace_info);
            return true;
          case BASIS:
            dispatch_helper<Basis>(handler, message, buffer_stack, trace_info);
            return true;
          case INDEX:
            dispatch_helper<Index>(handler, message, buffer_stack, trace_info);
            return true;
        }
        break;
      }
      case UNKNOWN_INTERNAL:
        if (allow_unknown_event_types) {
          return false;
        }
        break;
      case OK:
        if (!std::empty(frame.subbed)) {
          auto subbed = Subbed{
              .id = frame.id,
              .subbed = frame.subbed,
              .ts = frame.ts,
              .status = frame.status,
          };
          create_trace_and_dispatch(handler, trace_info, subbed);
          return true;
        }
        break;
      case ERROR:
        auto error = Error{
            .id = frame.id,
            .err_code = frame.err_code,
            .err_msg = frame.err_msg,
            .ts = frame.ts,
        };
        create_trace_and_dispatch(handler, trace_info, error);
        return true;
    }
  } else {
    dispatch_helper<Ping>(handler, message, buffer_stack, trace_info);
    return true;
  }
  log::fatal(R"(Unexpected: message="{}")"sv, message);
}

}  // namespace json
}  // namespace htx_futures
}  // namespace roq
