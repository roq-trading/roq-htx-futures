/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/application.hpp"

#include "roq/io/engine/context_factory.hpp"

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
  log::info("Prepare environment"sv);
  auto context = io::engine::ContextFactory::create(server::Flags::io_backend());
  log::info("Starting the gateway"sv);
  server::Settings settings{
      .package_name = ROQ_PACKAGE_NAME,
      .build_number = ROQ_BUILD_NUMBER,
      .api = Flags::api(),
      .type = server::Type::ORDER_MANAGEMENT,
  };
  server::Trading<Gateway>(settings, config, *context).dispatch();
  return EXIT_SUCCESS;
}

}  // namespace huobi_futures
}  // namespace roq
