/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <fmt/compile.h>
#include <fmt/format.h>

#include "roq/server.hpp"

#include "roq/huobi_futures/flags/common.hpp"
#include "roq/huobi_futures/flags/rest.hpp"
#include "roq/huobi_futures/flags/ws.hpp"

namespace roq {
namespace huobi_futures {

struct Settings final : public server::Settings {
  explicit Settings(server::Type);

  std::string_view exchange;

  flags::Common__flags common;
  flags::REST__flags rest;
  flags::WS__flags ws;
};

}  // namespace huobi_futures
}  // namespace roq

template <>
struct fmt::formatter<roq::huobi_futures::Settings> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(roq::huobi_futures::Settings const &value, Context &context) const {
    using namespace fmt::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(exchange="{}", )"
        R"(common={}, )"
        R"(rest={}, )"
        R"(ws={}, )"
        R"(server={})"
        R"(}})"_cf,
        value.exchange,
        value.common,
        value.rest,
        value.ws,
        static_cast<roq::server::Settings const &>(value));
  }
};
