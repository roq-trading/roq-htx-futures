/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat/fmt.hpp"

#include <fmt/format.h>

#include "roq/server/flags/settings.hpp"

#include "roq/huobi_futures/flags/flags.hpp"
#include "roq/huobi_futures/flags/misc.hpp"
#include "roq/huobi_futures/flags/rest.hpp"
#include "roq/huobi_futures/flags/ws.hpp"

namespace roq {
namespace huobi_futures {

struct Settings final : public server::flags::Settings {
  explicit Settings(args::Parser const &);

  std::string_view exchange;

  flags::Misc misc;
  flags::REST rest;
  flags::WS ws;

 private:
  Settings(args::Parser const &, flags::Flags const &);
};

}  // namespace huobi_futures
}  // namespace roq

template <>
struct fmt::formatter<roq::huobi_futures::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::huobi_futures::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(exchange="{}", )"
        R"(misc={}, )"
        R"(rest={}, )"
        R"(ws={}, )"
        R"(server={})"
        R"(}})"sv,
        value.exchange,
        value.misc,
        value.rest,
        value.ws,
        static_cast<roq::server::Settings const &>(value));
  }
};
