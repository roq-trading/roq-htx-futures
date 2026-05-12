/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/json/encoder.hpp"

#include "roq/logging.hpp"

#include "roq/decimal.hpp"

#include "roq/utils/charconv/from_chars.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/htx_futures/json/map.hpp"
#include "roq/htx_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace json {

// === CONSTANTS ===

namespace {
constexpr auto const OP_CREATE_ORDER = "P"sv;
constexpr auto const OP_CANCEL_ORDER = "C"sv;
constexpr auto const OP_CANCEL_ALL_ORDERS = "X"sv;
}  // namespace

// === IMPLEMENTATION ===

// REST

// lever_rate
// self_match_prevent
// stop-loss ??? => sl_
std::string_view Encoder::create_order(
    std::string &buffer,
    CreateOrder const &create_order,
    server::oms::Order const &,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id) {
  buffer.clear();
  auto direction = map(create_order.side).template get<json::Direction>();
  auto order_price_type = map(create_order.order_type, create_order.time_in_force, create_order.execution_instructions).template get<json::OrderPriceType>();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("contract_code":"{}",)"
      R"("client_order_id":"{}",)"
      R"("direction":"{}",)"
      R"("order_price_type":"{}",)"
      R"("volume":"{}")"sv,
      create_order.symbol,
      request_id,
      direction.as_raw_text(),
      order_price_type.as_raw_text(),
      Decimal{create_order.quantity, ref_data.quantity.precision});
  if (create_order.position_effect != PositionEffect{}) {
    auto offset = map(create_order.position_effect).template get<json::Offset>();
    fmt::format_to(std::back_inserter(buffer), R"(,"offset":"{}")"sv, offset.as_raw_text());
  }
  if (!std::isnan(create_order.price)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"price":"{}")"sv, Decimal{create_order.price, ref_data.price.precision});
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
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("contract_code":"{}")"sv,
      order.symbol);
  if (std::empty(order.external_order_id)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"client_order_id":"{}")"sv, order.client_order_id);
  } else {
    fmt::format_to(std::back_inserter(buffer), R"(,"order_id":"{}")"sv, order.external_order_id);
  }
  fmt::format_to(std::back_inserter(buffer), R"(}})"sv);
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

// WS

std::string_view Encoder::create_order_ws(
    std::string &buffer,
    CreateOrder const &create_order,
    server::oms::Order const &,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    MarginMode margin_mode) {
  auto op = [&]() -> std::string_view {
    switch (margin_mode) {
      using enum MarginMode;
      case UNDEFINED:
        break;
      case ISOLATED:
        return "create_order"sv;
      case CROSS:
        return "create_cross_order"sv;
      case PORTFOLIO:
        break;
    }
    log::fatal("Unexpected"sv);
  }();
  buffer.clear();
  auto direction = map(create_order.side).template get<json::Direction>();
  auto order_price_type = map(create_order.order_type, create_order.time_in_force, create_order.execution_instructions).template get<json::OrderPriceType>();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("op":"{}",)"
      R"("cid":"{}:{}:1",)"
      R"("data":{{)"
      R"("contract_code":"{}",)"
      R"("client_order_id":"{}",)"
      R"("direction":"{}",)"
      R"("order_price_type":"{}",)"
      R"("volume":"{}")"sv,
      op,
      OP_CREATE_ORDER,
      request_id,
      create_order.symbol,
      request_id,
      direction.as_raw_text(),
      order_price_type.as_raw_text(),
      Decimal{create_order.quantity, ref_data.quantity.precision});
  if (create_order.position_effect != PositionEffect{}) {
    auto offset = map(create_order.position_effect).template get<json::Offset>();
    fmt::format_to(std::back_inserter(buffer), R"(,"offset":"{}")"sv, offset.as_raw_text());
  }
  if (!std::isnan(create_order.price)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"price":"{}")"sv, Decimal{create_order.price, ref_data.price.precision});
  }
  if (!std::isnan(create_order.leverage)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"lever_rate":{})"sv, create_order.leverage);
  } else {
    fmt::format_to(std::back_inserter(buffer), R"(,"lever_rate":1)"sv);  // XXX FIXME TODO is this correct ???
  }
  fmt::format_to(
      std::back_inserter(buffer),
      R"(}})"
      R"(}})"sv);
  return buffer;
}

std::string_view Encoder::cancel_order_ws(
    std::string &buffer,
    CancelOrder const &cancel_order,
    server::oms::Order const &order,
    server::oms::RefData const &,
    std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id,
    MarginMode margin_mode) {
  auto op = [&]() -> std::string_view {
    switch (margin_mode) {
      using enum MarginMode;
      case UNDEFINED:
        break;
      case ISOLATED:
        return "cancel"sv;
      case CROSS:
        return "cross_cancel"sv;
      case PORTFOLIO:
        break;
    }
    log::fatal("Unexpected"sv);
  }();
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("op":"{}",)"
      R"("cid":"{}:{}:{}",)"
      R"("data":{{)"
      R"("contract_code":"{}")"sv,
      op,
      OP_CANCEL_ORDER,
      request_id,
      cancel_order.version,
      order.symbol);
  if (std::empty(order.external_order_id)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"client_order_id":"{}")"sv, order.client_order_id);
  } else {
    fmt::format_to(std::back_inserter(buffer), R"(,"order_id":"{}")"sv, order.external_order_id);
  }
  fmt::format_to(
      std::back_inserter(buffer),
      R"(}})"
      R"(}})"sv);
  return buffer;
}

std::string_view Encoder::cancel_all_orders_ws(
    std::string &buffer, CancelAllOrders const &, std::string_view const &request_id, std::string_view const &symbol, MarginMode margin_mode) {
  auto op = [&]() -> std::string_view {
    switch (margin_mode) {
      using enum MarginMode;
      case UNDEFINED:
        break;
      case ISOLATED:
        return "cancelall"sv;
      case CROSS:
        return "cross_cancelall"sv;
      case PORTFOLIO:
        break;
    }
    log::fatal("Unexpected"sv);
  }();
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("op":"{}",)"
      R"("cid":"{}:{}:0",)"
      R"("data":{{)"
      R"("contract_code":"{}")"
      R"(}})"
      R"(}})"sv,
      op,
      OP_CANCEL_ALL_ORDERS,
      request_id,
      symbol);
  return buffer;
}

std::tuple<RequestType, std::string_view, uint32_t> Encoder::split_cid(std::string_view const &cid) {
  assert(!std::empty(cid));
  auto pos_1 = cid.find(':');
  if (pos_1 != std::string_view::npos) {
    auto op = cid.substr(0, pos_1);
    auto request_type = [&]() -> RequestType {
      auto op_2 = utils::hash::FNV::compute(op);
      switch (op_2) {
        case utils::hash::FNV::compute(OP_CREATE_ORDER):
          return RequestType::CREATE_ORDER;
        case utils::hash::FNV::compute(OP_CANCEL_ORDER):
          return RequestType::CANCEL_ORDER;
      }
      return {};
    }();
    ++pos_1;
    auto pos_2 = cid.find(':', pos_1);
    if (pos_2 != std::string_view::npos) {
      auto request_id = cid.substr(pos_1, pos_2 - pos_1);
      ++pos_2;
      auto version = cid.substr(pos_2);
      auto version_2 = utils::charconv::from_chars<uint32_t>(version);
      return {
          request_type,
          request_id,
          version_2,
      };
    }
  }
  return {};
}

}  // namespace json
}  // namespace htx_futures
}  // namespace roq
