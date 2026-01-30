/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/application.hpp"

#include "roq/htx_futures/config.hpp"
#include "roq/htx_futures/gateway.hpp"
#include "roq/htx_futures/settings.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {

// === CONSTANTS ===

namespace {
uint8_t const API_USDT_M_FUTURES = 0x0;
uint8_t const API_COIN_M_DELIVERY = 0x1;
uint8_t const API_COIN_M_PERPETUAL = 0x2;
}  // namespace

// === HELPERS ===

namespace {
auto parse_api(auto &settings) {
  auto api = API::parse_api(settings);
  switch (api) {
    using enum API::Key;
    case USDT_M_FUTURES:
      return API_USDT_M_FUTURES;
    case COIN_M_DELIVERY:
      return API_COIN_M_DELIVERY;
    case COIN_M_PERPETUAL:
      return API_COIN_M_PERPETUAL;
  }
  log::fatal(R"(Unexpected: api="{}")"sv, settings.app.api);
}
}  // namespace

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  auto api = parse_api(settings);
  Config config{settings};
  auto context = server::create_io_context(settings);
  server::Trading<Gateway>{settings, config, *context, api}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace htx_futures
}  // namespace roq
