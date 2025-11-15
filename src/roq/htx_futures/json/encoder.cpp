/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/json/encoder.hpp"

#include "roq/htx_futures/json/map.hpp"
#include "roq/htx_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace json {

// === IMPLEMENTATION ===

std::string_view Encoder::place_order(std::string &buffer, CreateOrder const &create_order, server::oms::Order const &, std::string_view const &request_id) {
  buffer.clear();
  return buffer;
}

std::string_view Encoder::modify_order(
    std::string &buffer,
    ModifyOrder const &modify_order,
    server::oms::Order const &,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  buffer.clear();
  return buffer;
}

std::string_view Encoder::cancel_order(
    std::string &buffer,
    CancelOrder const &,
    server::oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  buffer.clear();
  return buffer;
}

std::string_view Encoder::cancel_all_orders(std::string &buffer, CancelAllOrders const &, [[maybe_unused]] std::string_view const &request_id) {
  buffer.clear();
  return buffer;
}

}  // namespace json
}  // namespace htx_futures
}  // namespace roq
