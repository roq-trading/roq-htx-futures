/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/application.hpp"

#include "roq/huobi_futures/config.hpp"
#include "roq/huobi_futures/flags.hpp"
#include "roq/huobi_futures/gateway.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

// === HELPERS ===

namespace {
auto get_settings = []() {
  return server::Settings{
      .package_name = ROQ_PACKAGE_NAME,
      .build_number = ROQ_BUILD_NUMBER,
      .api = Flags::api(),
      .type = server::Type::ORDER_MANAGEMENT,
  };
};
}

// === IMPLEMENTATION ===

int Application::main(int, char **) {
  Config config;
  auto context = server::create_io_context();
  auto settings = get_settings();
  server::Trading<Gateway>(settings, config, *context).dispatch();
  return EXIT_SUCCESS;
}

}  // namespace huobi_futures
}  // namespace roq
