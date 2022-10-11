/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/application.hpp"

using namespace std::literals;

// === CONSTANTS ===

namespace {
roq::Service::Info const INFO{
    .description = "Roq Huobi Futures Gateway"sv,
    .package_name = ROQ_PACKAGE_NAME,
    .host = ROQ_HOST,
    .build_version = ROQ_BUILD_VERSION,
    .build_number = ROQ_BUILD_NUMBER,
    .build_type = ROQ_BUILD_TYPE,
    .git_hash = ROQ_GIT_DESCRIBE_HASH,
};
}  // namespace

int main(int argc, char **argv) {
  return roq::huobi_futures::Application(argc, argv, INFO).run();
}
