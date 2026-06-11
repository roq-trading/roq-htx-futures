/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/htx_futures/protocol/json/direction.hpp"
#include "roq/htx_futures/protocol/json/event.hpp"
#include "roq/htx_futures/protocol/json/offset.hpp"
#include "roq/htx_futures/protocol/json/order_price_type.hpp"
#include "roq/htx_futures/protocol/json/role.hpp"

#include "roq/execution_instruction.hpp"
#include "roq/liquidity.hpp"
#include "roq/order_type.hpp"
#include "roq/position_effect.hpp"
#include "roq/side.hpp"
#include "roq/time_in_force.hpp"
#include "roq/update_type.hpp"

#include "roq/map.hpp"

#include "roq/mask.hpp"

namespace roq {

template <>
template <>
std::optional<Side> Map<htx_futures::protocol::json::Direction>::helper() const;

template <>
template <>
std::optional<UpdateType> Map<htx_futures::protocol::json::Event>::helper() const;

template <>
template <>
std::optional<PositionEffect> Map<htx_futures::protocol::json::Offset>::helper() const;

template <>
template <>
std::optional<OrderType> Map<htx_futures::protocol::json::OrderPriceType>::helper() const;

template <>
template <>
std::optional<Liquidity> Map<htx_futures::protocol::json::Role>::helper() const;

template <>
template <>
std::optional<OrderStatus> Map<std::int32_t>::helper() const;

// ===

template <>
template <>
std::optional<htx_futures::protocol::json::OrderPriceType> Map<roq::OrderType, roq::TimeInForce, Mask<roq::ExecutionInstruction>>::helper() const;

template <>
template <>
std::optional<htx_futures::protocol::json::Offset> Map<roq::PositionEffect>::helper() const;

template <>
template <>
std::optional<htx_futures::protocol::json::Direction> Map<roq::Side>::helper() const;

}  // namespace roq
