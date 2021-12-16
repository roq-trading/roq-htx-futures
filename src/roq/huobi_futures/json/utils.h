/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/core/utility.h"

#include "roq/core/json/parser.h"

#include "roq/core/charconv/datetime.h"

#include "roq/huobi_futures/json/side.h"

namespace roq {
namespace huobi_futures {
namespace json {

template <typename T>
inline void update(T &result, const core::json::value_t &value) {
  result = core::json::get<T>(value);
}

template <>
inline void update(std::chrono::milliseconds &result, const core::json::value_t &value) {
  return std::visit(
      overloaded{
          [&](const core::json::null_t &) { result = std::chrono::milliseconds{}; },
          [](bool) { throw std::bad_cast(); },
          [&](int64_t value) { result = std::chrono::milliseconds{value}; },
          [&](double value) { result = std::chrono::milliseconds{static_cast<int64_t>(value)}; },
          [&](const std::string_view &value) {
            result =
                core::charconv::datetime_from_string<std::remove_reference<decltype(result)>::type>(
                    value);
          },
          [](const core::json::object_t &) { throw std::bad_cast(); },
          [](const core::json::array_t &) { throw std::bad_cast(); },
      },
      value);
}

inline std::string_view extract_symbol(const std::string_view &channel) {
  auto sep1 = channel.find_first_of('.');
  if (sep1 != channel.npos) {
    auto sep2 = channel.find_first_of('.', sep1 + 1);
    if (sep2 != channel.npos)
      return channel.substr(sep1 + 1, sep2 - sep1 - 1);
    return channel.substr(sep1 + 1);
  }
  return channel;
}

inline std::string_view extract_topic(const std::string_view &channel) {
  auto sep1 = channel.find_first_of('.');
  if (sep1 != channel.npos) {
    auto sep2 = channel.find_first_of('.', sep1 + 1);
    if (sep2 != channel.npos) {
      auto sep3 = channel.find_first_of('.', sep2 + 1);
      if (sep3 != channel.npos)
        return channel.substr(sep2 + 1, sep3 - sep2 - 1);
      return channel.substr(sep2 + 1);
    }
    return channel.substr(sep1 + 1);
  }
  return channel;
}

inline roq::Side map(json::Side side) {
  switch (side) {
    case json::Side::UNDEFINED:
      break;
    case json::Side::UNKNOWN:
      break;
    case json::Side::BUY:
      return roq::Side::BUY;
    case json::Side::SELL:
      return roq::Side::SELL;
  }
  return roq::Side::UNDEFINED;
}

inline json::Side map(roq::Side side) {
  switch (side) {
    case roq::Side::UNDEFINED:
      break;
    case roq::Side::BUY:
      return json::Side::BUY;
    case roq::Side::SELL:
      return json::Side::SELL;
  }
  return json::Side::UNDEFINED;
}

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
