/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/huobi_futures/application.hpp"

#include "roq/huobi_futures/config.hpp"
#include "roq/huobi_futures/gateway.hpp"
#include "roq/huobi_futures/settings.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

// === CONSTANTS ===

namespace {
auto const TYPE = server::Type::ORDER_MANAGEMENT;
}

// === IMPLEMENTATION ===

int Application::main(args::Parser const &) {
  Settings settings{TYPE};
  Config config{settings};
  auto context = server::create_io_context(settings);
  server::Trading<Gateway>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace huobi_futures
}  // namespace roq
