/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi_futures/application.h"

#include "roq/literals.h"

using namespace roq::literals;

namespace {
static const auto DESCRIPTION = "Roq Binance Gateway"_sv;
}  // namespace

int main(int argc, char **argv) {
  return roq::huobi_futures::Application(
             argc, argv, DESCRIPTION, ROQ_BUILD_VERSION, ROQ_BUILD_TYPE, ROQ_GIT_DESCRIBE_HASH)
      .run();
}
