/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/cancel_all_orders.hpp"
#include "roq/cancel_order.hpp"
#include "roq/create_order.hpp"
#include "roq/modify_order.hpp"

#include "roq/request_type.hpp"

#include "roq/server/oms/order.hpp"
#include "roq/server/oms/ref_data.hpp"

namespace roq {
namespace htx_futures {
namespace protocol {
namespace json {

struct Encoder final {
  // --- v1 ---

  // REST
  static std::string_view create_order(
      std::string &buffer, CreateOrder const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id);

  static std::string_view cancel_order(
      std::string &buffer,
      CancelOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  static std::string_view cancel_all_orders(std::string &buffer, CancelAllOrders const &, std::string_view const &request_id, std::string_view const &symbol);

  // WS
  static std::string_view create_order_ws(
      std::string &buffer, CreateOrder const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id, roq::MarginMode);

  static std::string_view cancel_order_ws(
      std::string &buffer,
      CancelOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id,
      roq::MarginMode);

  static std::string_view cancel_all_orders_ws(
      std::string &buffer, CancelAllOrders const &, std::string_view const &request_id, std::string_view const &symbol, roq::MarginMode);

  static std::tuple<RequestType, std::string_view, uint32_t> split_cid(std::string_view const &cid);

  // --- v5 ---

  // REST
  static std::string_view create_order_v5(
      std::string &buffer, CreateOrder const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id, roq::MarginMode);

  // WS
  static std::string_view create_order_ws_v5(
      std::string &buffer, CreateOrder const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id, roq::MarginMode);

  static std::string_view cancel_order_ws_v5(
      std::string &buffer,
      CancelOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  static std::string_view cancel_all_orders_ws_v5(
      std::string &buffer, CancelAllOrders const &, std::string_view const &request_id, std::string_view const &symbol);
};

}  // namespace json
}  // namespace protocol
}  // namespace htx_futures
}  // namespace roq
