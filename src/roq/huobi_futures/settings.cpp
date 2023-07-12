/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/huobi_futures/settings.hpp"

#include "roq/logging.hpp"

#include "roq/huobi_futures/flags/flags.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

Settings::Settings(args::Parser const &args, server::Type type)
    : server::flags::Settings{args, type, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER, flags::Flags::api()},
      exchange{flags::Flags::exchange()} {
  log::debug("settings={}"sv, *this);
}

}  // namespace huobi_futures
}  // namespace roq
