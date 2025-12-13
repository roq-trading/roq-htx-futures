/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/settings.hpp"

#include "roq/logging.hpp"

#include "roq/utils/enum.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {

// === HELPERS ===

namespace {
auto create_margin_mode(auto &value) -> MarginMode {
  if (std::empty(value)) {
    return {};
  }
  return utils::parse_enum<MarginMode>(value);
}
}  // namespace

// === imlementation ===

Settings::Settings(args::Parser const &args) : Settings{args, flags::Flags::create()} {
}

Settings::Settings(args::Parser const &args, flags::Flags const &flags)
    : server::flags::Settings{args, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER, flags.api}, exchange{flags.exchange}, ws_api{flags.ws_api},
      margin_mode{create_margin_mode(flags.margin_mode)}, misc{flags::Misc::create()}, rest{flags::REST::create()}, ws{flags::WS::create()} {
  log::info("settings={}"sv, *this);
}

}  // namespace htx_futures
}  // namespace roq
