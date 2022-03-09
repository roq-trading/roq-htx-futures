/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/application.hpp"

#include "roq/huobi_futures/config.hpp"
#include "roq/huobi_futures/flags.hpp"
#include "roq/huobi_futures/gateway.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

int Application::main(int, char **) {
  log::info(R"(Parse config_file="{}")"sv, Flags::config_file());
  Config config(Flags::config_file(), Flags::secrets_file());
  log::info<1>("config={}"sv, config);
  log::info("Starting the gateway"sv);
  roq::server::Trading<Gateway>(ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER, Flags::api(), config)
      .dispatch();
  return EXIT_SUCCESS;
}

}  // namespace huobi_futures
}  // namespace roq
