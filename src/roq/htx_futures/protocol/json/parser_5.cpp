/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/protocol/json/parser_5.hpp"

#include "roq/utils/compare.hpp"

#include "roq/logging.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/htx_futures/protocol/json/topic_5.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace protocol {
namespace json {

// === HELPERS ===

namespace {
constexpr auto const KEY_OP = "op"sv;
constexpr auto const KEY_CID = "cid"sv;
constexpr auto const KEY_TOPIC = "topic"sv;
}  // namespace

// === HELPERS ===

namespace {
template <typename T>
auto dispatch_helper(auto &handler, auto &message, auto &buffer_stack, auto &trace_info) {
  T obj{message, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj);
  return true;
}
}  // namespace

// === IMPLEMENTATION ===

bool Parser5::dispatch(
    Parser5::Handler &handler,
    std::string_view const &message,
    core::json::BufferStack &buffer_stack,
    TraceInfo const &trace_info,
    bool allow_unknown_event_types) {
  auto result = false;
  auto helper = [&](auto &key, auto &value) {
    auto key_2 = utils::hash::FNV::compute(key);
    switch (key_2) {
      case utils::hash::FNV::compute(KEY_OP): {
        Operator op{value};
        switch (op) {
          using enum Operator::type_t;
          case UNDEFINED_INTERNAL:
            log::fatal("Unexpected"sv);
          case UNKNOWN_INTERNAL:
            break;
          case CLOSE:
            result = dispatch_helper<Close2>(handler, message, buffer_stack, trace_info);
            break;
          case ERROR:
            result = dispatch_helper<Error2>(handler, message, buffer_stack, trace_info);
            break;
          case PING:
            result = dispatch_helper<Ping>(handler, message, buffer_stack, trace_info);
            break;
          case AUTH:
            result = dispatch_helper<Auth>(handler, message, buffer_stack, trace_info);
            break;
          case SUB:
            result = dispatch_helper<Sub>(handler, message, buffer_stack, trace_info);
            break;
          case NOTIFY:
            return false;  // note! continue
        }
        return true;
      }
      case utils::hash::FNV::compute(KEY_CID): {
        result = dispatch_helper<Response5>(handler, message, buffer_stack, trace_info);
        break;
      }
      case utils::hash::FNV::compute(KEY_TOPIC): {
        Topic5 topic{value};
        switch (topic) {
          using enum Topic5::type_t;
          case UNDEFINED_INTERNAL:
            break;
          case UNKNOWN_INTERNAL:
            break;
          case ACCOUNT:
            result = dispatch_helper<Account5>(handler, message, buffer_stack, trace_info);
            break;
          case POSITIONS:
            result = dispatch_helper<Positions5>(handler, message, buffer_stack, trace_info);
            break;
          case MATCH_ORDERS:
            result = dispatch_helper<MatchOrders5>(handler, message, buffer_stack, trace_info);
            break;
          case ORDERS:
            result = dispatch_helper<Orders5>(handler, message, buffer_stack, trace_info);
            break;
        }
        return true;
      }
    }
    return result;
  };
  core::json::Parser::dispatch<core::json::Object>(helper, message);
  if (result || allow_unknown_event_types) {
    return result;
  }
  log::fatal(R"(Unexpected: message="{}")"sv, message);
}

}  // namespace json
}  // namespace protocol
}  // namespace htx_futures
}  // namespace roq
