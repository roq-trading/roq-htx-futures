/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/huobi_futures/json/basis.hpp"

using namespace roq;
using namespace roq::huobi_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_basis_simple_swap", "[json_basis]") {
  auto message = R"({)"
                 R"("ch":"market.WOO-USDT.basis.1min.open",)"
                 R"("ts":1642659617542,)"
                 R"("tick":{)"
                 R"("id":1642659600,)"
                 R"("index_price":"0.8645621602666667",)"
                 R"("contract_price":"0.851",)"
                 R"("basis":"-0.0135621602666667",)"
                 R"("basis_rate":"-0.0156867382010838518425939816818336619"})"
                 R"(})";
  std::vector<std::byte> buffer(8192);
  auto obj = json::Basis::create(message, buffer);
  CHECK(obj.ch == "market.WOO-USDT.basis.1min.open"sv);
  CHECK(obj.ts == 1642659617542ms);
  auto &tick = obj.tick;
  CHECK(tick.id == 1642659600);
  CHECK(tick.index_price == 0.8645621602666667_a);
  CHECK(tick.contract_price == 0.851_a);
  CHECK(tick.basis == -0.0135621602666667_a);
  CHECK(tick.basis_rate == -0.0156867382010838518425939816818336619_a);
}
