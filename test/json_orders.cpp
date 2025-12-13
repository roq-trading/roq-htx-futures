/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_2_tester.hpp"

using namespace roq;
using namespace roq::htx_futures;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::OrdersCross;

TEST_CASE("init", "[json_orders_cross]") {
  auto message = R"({)"
                 R"("contract_type":"swap",)"
                 R"("pair":"APT-USDT",)"
                 R"("business_type":"swap",)"
                 R"("op":"notify",)"
                 R"("topic":"orders_cross.apt-usdt",)"
                 R"("ts":1764847495245,)"
                 R"("symbol":"APT",)"
                 R"("contract_code":"APT-USDT",)"
                 R"("volume":1,)"
                 R"("price":1.0000,)"
                 R"("order_price_type":"limit",)"
                 R"("direction":"buy",)"
                 R"("offset":"open",)"
                 R"("status":7,)"
                 R"("lever_rate":1,)"
                 R"("order_id":1446217475728912385,)"
                 R"("order_id_str":"1446217475728912385",)"
                 R"("client_order_id":563241524409083,)"
                 R"("order_source":"api",)"
                 R"("order_type":2,)"
                 R"("created_at":1764846715627,)"
                 R"("trade_volume":0,)"
                 R"("trade_turnover":0,)"
                 R"("fee":0,)"
                 R"("trade_avg_price":0.0000,)"
                 R"("margin_frozen":0E-18,)"
                 R"("profit":0,)"
                 R"("trade":[],)"
                 R"("canceled_at":1764847495233,)"
                 R"("fee_asset":"USDT",)"
                 R"("margin_asset":"USDT",)"
                 R"("uid":"573242943",)"
                 R"("liquidation_type":"0",)"
                 R"("margin_mode":"cross",)"
                 R"("margin_account":"USDT",)"
                 R"("is_tpsl":0,)"
                 R"("real_profit":0,)"
                 R"("trade_partition":"USDT",)"
                 R"("reduce_only":0,)"
                 R"("self-match-prevent":1,)"
                 R"("self_match_prevent_new":"cancel_taker",)"
                 R"("canceled_source":"api")"
                 R"(})";
  auto helper = [](value_type const &obj) { CHECK(obj.op == json::Operator::NOTIFY); };
  Parser2Tester<value_type>::dispatch(helper, message, 8192, 1);
}
