/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/utils/patterns.hpp"

#include "roq/utils/charconv/from_chars.hpp"

#include "roq/core/json/parser.hpp"

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
            result = utils::charconv::from_chars<std::remove_reference<decltype(result)>::type>(val, utils::charconv::Format::DATETIME);
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
    if (sep2 != channel.npos) {
      return channel.substr(sep1 + 1, sep2 - sep1 - 1);
    }
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
      if (sep3 != channel.npos) {
        return channel.substr(sep2 + 1, sep3 - sep2 - 1);
      }
      return channel.substr(sep2 + 1);
    }
    return channel.substr(sep1 + 1);
  }
  return channel;
}

}  // namespace json
}  // namespace huobi_futures
}  // namespace roq
