/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/fix_bridge/settings.hpp"

#include "roq/server/fix_bridge/flags/settings.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace fix_bridge {

// === HELPERS ===

namespace {
auto create_fix_bridge(auto &parser) {
  return server::fix_bridge::flags::Settings{parser};
}
}  // namespace

// === IMPLEMENTATION ===

Settings::Settings(args::Parser const &parser) : flags::Settings{parser}, fix_bridge{create_fix_bridge(parser)} {
}

}  // namespace fix_bridge
}  // namespace htx_futures
}  // namespace roq
