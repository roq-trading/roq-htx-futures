/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/htx_futures/protocol/json/direction.hpp"
#include "roq/htx_futures/protocol/json/event.hpp"
#include "roq/htx_futures/protocol/json/event5.hpp"
#include "roq/htx_futures/protocol/json/margin_mode.hpp"
#include "roq/htx_futures/protocol/json/offset.hpp"
#include "roq/htx_futures/protocol/json/order_price_type.hpp"
#include "roq/htx_futures/protocol/json/order_state.hpp"
#include "roq/htx_futures/protocol/json/order_type.hpp"
#include "roq/htx_futures/protocol/json/position_side.hpp"
#include "roq/htx_futures/protocol/json/role.hpp"
#include "roq/htx_futures/protocol/json/side.hpp"
#include "roq/htx_futures/protocol/json/time_in_force.hpp"

#include "roq/execution_instruction.hpp"
#include "roq/liquidity.hpp"
#include "roq/margin_mode.hpp"
#include "roq/order_status.hpp"
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
std::optional<UpdateType> Map<htx_futures::protocol::json::Event5>::helper() const;

template <>
template <>
std::optional<roq::MarginMode> Map<htx_futures::protocol::json::MarginMode>::helper() const;

template <>
template <>
std::optional<PositionEffect> Map<htx_futures::protocol::json::Offset>::helper() const;

template <>
template <>
std::optional<roq::OrderType> Map<htx_futures::protocol::json::OrderPriceType>::helper() const;

template <>
template <>
std::optional<roq::OrderStatus> Map<htx_futures::protocol::json::OrderState>::helper() const;

template <>
template <>
std::optional<roq::OrderType> Map<htx_futures::protocol::json::OrderType>::helper() const;

template <>
template <>
std::optional<PositionEffect> Map<htx_futures::protocol::json::PositionSide, htx_futures::protocol::json::Side>::helper() const;

template <>
template <>
std::optional<Liquidity> Map<htx_futures::protocol::json::Role>::helper() const;

template <>
template <>
std::optional<Side> Map<htx_futures::protocol::json::Side>::helper() const;

template <>
template <>
std::optional<OrderStatus> Map<std::int32_t>::helper() const;

// ===

template <>
template <>
std::optional<htx_futures::protocol::json::MarginMode> Map<roq::MarginMode>::helper() const;

template <>
template <>
std::optional<htx_futures::protocol::json::OrderPriceType> Map<roq::OrderType, roq::TimeInForce, Mask<roq::ExecutionInstruction>>::helper() const;

template <>
template <>
std::optional<htx_futures::protocol::json::OrderType> Map<roq::OrderType, Mask<roq::ExecutionInstruction>>::helper() const;

template <>
template <>
std::optional<htx_futures::protocol::json::Offset> Map<roq::PositionEffect>::helper() const;

template <>
template <>
std::optional<htx_futures::protocol::json::PositionSide> Map<roq::PositionEffect>::helper() const;

template <>
template <>
std::optional<htx_futures::protocol::json::Direction> Map<roq::Side>::helper() const;

template <>
template <>
std::optional<htx_futures::protocol::json::Side> Map<roq::Side>::helper() const;

template <>
template <>
std::optional<htx_futures::protocol::json::TimeInForce> Map<roq::TimeInForce>::helper() const;

}  // namespace roq
