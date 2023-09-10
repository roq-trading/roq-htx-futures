/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/utils/patterns.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/charconv/datetime.hpp"

#include "roq/huobi_futures/json/side.hpp"

namespace roq {
namespace huobi_futures {
namespace json {

template <typename T>
inline void update(T &result, core::json::Value const &value) {
  result = core::json::get<T>(value);
}

template <>
inline void update(std::chrono::milliseconds &result, core::json::Value const &value) {
  return std::visit(
      utils::overloaded{
          [&](core::json::Null const &) { result = std::chrono::milliseconds{}; },
          [](bool) { throw std::bad_cast{}; },
          [&](int64_t val) { result = std::chrono::milliseconds{val}; },
          [&](double val) { result = std::chrono::milliseconds{static_cast<int64_t>(val)}; },
          [&](std::string_view const &val) {
            result = core::charconv::datetime_from_string<std::remove_reference<decltype(result)>::type>(val);
          },
          [](core::json::Object const &) { throw std::bad_cast{}; },
          [](core::json::Array const &) { throw std::bad_cast{}; },
      },
      value);
}

inline std::string_view extract_symbol(std::string_view const &channel) {
  auto sep1 = channel.find_first_of('.');
  if (sep1 != channel.npos) {
    auto sep2 = channel.find_first_of('.', sep1 + 1);
    if (sep2 != channel.npos)
      return channel.substr(sep1 + 1, sep2 - sep1 - 1);
    return channel.substr(sep1 + 1);
  }
  return channel;
}

inline std::string_view extract_topic(std::string_view const &channel) {
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
    using enum json::Side::type_t;
    case UNDEFINED__:
    case UNKNOWN__:
      break;
    case BUY:
      return roq::Side::BUY;
    case SELL:
      return roq::Side::SELL;
  }
  return {};
}

inline json::Side map(roq::Side side) {
  switch (side) {
    using enum roq::Side;
    case UNDEFINED:
      break;
    case BUY:
      return json::Side::BUY;
    case SELL:
      return json::Side::SELL;
  }
  return {};
}

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
