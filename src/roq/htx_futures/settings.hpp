/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/margin_mode.hpp"

#include "roq/server/flags/settings.hpp"

#include "roq/htx_futures/flags/flags.hpp"
#include "roq/htx_futures/flags/misc.hpp"
#include "roq/htx_futures/flags/rest.hpp"
#include "roq/htx_futures/flags/ws.hpp"

namespace roq {
namespace htx_futures {

struct Settings final : public server::flags::Settings {
  explicit Settings(args::Parser const &);

  std::string_view exchange;
  bool ws_api = {};
  MarginMode margin_mode;

  flags::Misc misc;
  flags::REST rest;
  flags::WS ws;

 private:
  Settings(args::Parser const &, flags::Flags const &);
};

}  // namespace htx_futures
}  // namespace roq

template <>
struct fmt::formatter<roq::htx_futures::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::htx_futures::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(exchange="{}", )"
        R"(ws_api={}, )"
        R"(margin_mode={}, )"
        R"(misc={}, )"
        R"(rest={}, )"
        R"(ws={}, )"
        R"(server={})"
        R"(}})"sv,
        value.exchange,
        value.ws_api,
        value.margin_mode,
        value.misc,
        value.rest,
        value.ws,
        static_cast<roq::server::Settings const &>(value));
  }
};
