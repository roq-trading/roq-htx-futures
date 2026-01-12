/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/json/parser_3.hpp"

#include "roq/logging.hpp"

#include "roq/utils/hash/fnv.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace json {

// === HELPERS ===

namespace {
constexpr auto const KEY_OP = "op"sv;
constexpr auto const KEY_STATUS = "status"sv;
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

bool Parser3::dispatch(
    Parser3::Handler &handler,
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
            return false;  // note! unexpected
          case NOTIFY:
            return false;  // note! continue
        }
        return true;
      }
      case utils::hash::FNV::compute(KEY_STATUS): {
        result = dispatch_helper<Response>(handler, message, buffer_stack, trace_info);
        break;
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
}  // namespace htx_futures
}  // namespace roq
