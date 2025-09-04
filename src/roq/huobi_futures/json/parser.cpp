/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/huobi_futures/json/parser.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "roq/compat.hpp"

#include "roq/logging.hpp"

#include "roq/huobi_futures/json/frame.hpp"
#include "roq/huobi_futures/json/topic.hpp"
#include "roq/huobi_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {
namespace json {

bool Parser::dispatch(Parser::Handler &handler, std::string_view const &message, core::json::BufferStack &buffer_stack, TraceInfo const &trace_info) {
  Frame frame{message, buffer_stack};
  if (!frame.ping.count()) {
    switch (frame.status) {
      using enum Status::type_t;
      case UNDEFINED_INTERNAL: {
        Topic topic{extract_topic(frame.ch)};
        switch (topic) {
          using enum Topic::type_t;
          case BBO: {
            json::BBO bbo{message, buffer_stack};
            create_trace_and_dispatch(handler, trace_info, bbo);
            return true;
          }
          case DEPTH: {
            Depth depth{message, buffer_stack};
            create_trace_and_dispatch(handler, trace_info, depth);
            return true;
          }
          case TRADE: {
            Trade trade{message, buffer_stack};
            create_trace_and_dispatch(handler, trace_info, trade);
            return true;
          }
          case DETAIL: {
            Detail detail{message, buffer_stack};
            create_trace_and_dispatch(handler, trace_info, detail);
            return true;
          }
          case ESTIMATED_RATE: {
            EstimatedRate estimated_rate{message, buffer_stack};
            create_trace_and_dispatch(handler, trace_info, estimated_rate);
            return true;
          }
          case PREMIUM_INDEX: {
            PremiumIndex premium_index{message, buffer_stack};
            create_trace_and_dispatch(handler, trace_info, premium_index);
            return true;
          }
          case BASIS: {
            Basis basis{message, buffer_stack};
            create_trace_and_dispatch(handler, trace_info, basis);
            return true;
          }
          case INDEX: {
            Index index{message, buffer_stack};
            create_trace_and_dispatch(handler, trace_info, index);
            return true;
          }
          default:
            break;
        }
        break;
      }
      case UNKNOWN_INTERNAL:
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
        } else {
          log::fatal("DEBUG {}"sv, message);  // ???
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
    auto ping = Ping{
        .timestamp = frame.ping,
    };
    create_trace_and_dispatch(handler, trace_info, ping);
    return true;
  }
  log::warn(R"(Unexpected: message="{}")"sv, message);
  return false;
}

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
