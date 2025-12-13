/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <chrono>
#include <string>
#include <string_view>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <roq/utils/hash/sha256.hpp>

#include "roq/htx_futures/tools/crypto.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("simple", "[crypto]") {
  /*
  tools::Crypto crypto{};
  auto timestamp = 1637681707400ms;
  auto nonce = "sfx7kglb6r2outb74dnut65vlywu4csr"sv;
  auto [signature, used_timestamp] = crypto.create_signature(timestamp, nonce);
  CHECK(signature == "64064fb648aaa12eb60e87ed3410b18039bca746a670e684783389a1cd374e93"sv);
  */
}
