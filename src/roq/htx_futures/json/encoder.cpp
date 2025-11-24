/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/json/encoder.hpp"

#include "roq/decimal.hpp"

#include "roq/htx_futures/json/map.hpp"
#include "roq/htx_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace json {

// === IMPLEMENTATION ===

// lever_rate
// self_match_prevent
// stop-loss ??? => sl_
std::string_view Encoder::create_order(
    std::string &buffer, CreateOrder const &create_order, server::oms::Order const &order, std::string_view const &request_id) {
  buffer.clear();
  auto direction = map(create_order.side).template get<json::Side>();
  auto offset = map(create_order.position_effect).template get<json::Offset>();
  auto order_price_type = map(create_order.order_type).template get<json::OrderPriceType>();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("contract_code":"{}",)"
      R"("client_order_id":"{}",)"
      R"("direction":"{}",)"
      R"("offset":"{}",)"
      R"("order_price_type":"{}",)"
      R"("volume":"{}")"sv,
      create_order.symbol,
      request_id,
      direction.as_raw_text(),
      offset.as_raw_text(),
      order_price_type.as_raw_text(),
      Decimal{create_order.quantity, order.quantity_precision.precision});
  if (!std::isnan(create_order.price)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"price":"{}")"sv, Decimal{create_order.price, order.price_precision.precision});
  }
  if (!std::isnan(create_order.leverage)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"lever_rate":{})"sv, create_order.leverage);
  } else {
    fmt::format_to(std::back_inserter(buffer), R"(,"lever_rate":1)"sv);  // XXX FIXME TODO is this correct ???
  }
  fmt::format_to(std::back_inserter(buffer), R"(}})"sv);
  return buffer;
}

std::string_view Encoder::cancel_order(
    std::string &buffer,
    CancelOrder const &,
    server::oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("contract_code":"{}",)"sv,
      order.symbol);
  if (std::empty(order.external_order_id)) {
    fmt::format_to(
        std::back_inserter(buffer),
        R"("client_order_id":"{}")"
        R"(}})"sv,
        order.client_order_id);
  } else {
    fmt::format_to(
        std::back_inserter(buffer),
        R"("order_id":"{}")"
        R"(}})"sv,
        order.external_order_id);
  }
  return buffer;
}

std::string_view Encoder::cancel_all_orders(
    std::string &buffer, CancelAllOrders const &, [[maybe_unused]] std::string_view const &request_id, std::string_view const &symbol) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("contract_code":"{}")"
      R"(}})"sv,
      symbol);
  return buffer;
}

}  // namespace json
}  // namespace htx_futures
}  // namespace roq
