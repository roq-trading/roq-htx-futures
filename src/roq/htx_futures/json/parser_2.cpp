/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/json/parser_2.hpp"

#include "roq/utils/compare.hpp"

#include "roq/logging.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/htx_futures/json/topic_2.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace json {

// === HELPERS ===

namespace {
constexpr auto const KEY_OP = "op"sv;
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

constexpr auto extract_topic_helper(std::string_view const &topic) {
  auto separator_1 = topic.find_first_of('.');
  auto tmp_1 = topic.substr(0, separator_1);
  if (tmp_1 != "public"sv) {
    return tmp_1;
  }
  auto separator_2 = topic.find_last_of('.');
  auto tmp_2 = topic.substr(separator_2 == topic.npos ? 0 : (separator_2 + 1));
  return tmp_2;
}

static_assert(extract_topic_helper("public.BTC-USDT.funding_rate"sv) == "funding_rate"sv);
//
static_assert(extract_topic_helper("accounts"sv) == "accounts"sv);
static_assert(extract_topic_helper("positions"sv) == "positions"sv);
static_assert(extract_topic_helper("matchOrders.btc-usd"sv) == "matchOrders"sv);
//
static_assert(extract_topic_helper("accounts_cross"sv) == "accounts_cross"sv);
static_assert(extract_topic_helper("positions_cross"sv) == "positions_cross"sv);
static_assert(extract_topic_helper("matchOrders_cross.btc-usd"sv) == "matchOrders_cross"sv);

auto extract_topic(std::string_view const &topic) {
  auto tmp = extract_topic_helper(topic);
  return Topic2{tmp};
}
}  // namespace

// === IMPLEMENTATION ===

bool Parser2::dispatch(
    Parser2::Handler &handler,
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
      case utils::hash::FNV::compute(KEY_TOPIC): {
        auto topic = extract_topic(std::get<std::string_view>(value));
        switch (topic) {
          using enum Topic2::type_t;
          case UNDEFINED_INTERNAL:
            break;
          case UNKNOWN_INTERNAL:
            break;
          case FUNDING_RATE:
            result = dispatch_helper<FundingRate>(handler, message, buffer_stack, trace_info);
            break;
            //
          case ACCOUNTS:
            result = dispatch_helper<Accounts>(handler, message, buffer_stack, trace_info);
            break;
          case POSITIONS:
            result = dispatch_helper<Positions>(handler, message, buffer_stack, trace_info);
            break;
          case MATCH_ORDERS:
            result = dispatch_helper<MatchOrders>(handler, message, buffer_stack, trace_info);
            break;
          case ORDERS:
            result = dispatch_helper<Orders>(handler, message, buffer_stack, trace_info);
            break;
            //
          case ACCOUNTS_CROSS:
            result = dispatch_helper<AccountsCross>(handler, message, buffer_stack, trace_info);
            break;
          case POSITIONS_CROSS:
            result = dispatch_helper<PositionsCross>(handler, message, buffer_stack, trace_info);
            break;
          case MATCH_ORDERS_CROSS:
            result = dispatch_helper<MatchOrdersCross>(handler, message, buffer_stack, trace_info);
            break;
          case ORDERS_CROSS:
            result = dispatch_helper<OrdersCross>(handler, message, buffer_stack, trace_info);
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
}  // namespace htx_futures
}  // namespace roq
