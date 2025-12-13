/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/htx_futures/json/direction.hpp"
#include "roq/htx_futures/json/event.hpp"
#include "roq/htx_futures/json/offset.hpp"
#include "roq/htx_futures/json/order_price_type.hpp"
#include "roq/htx_futures/json/role.hpp"

#include "roq/liquidity.hpp"
#include "roq/order_type.hpp"
#include "roq/position_effect.hpp"
#include "roq/side.hpp"
#include "roq/update_type.hpp"

#include "roq/map.hpp"

namespace roq {

template <>
template <>
std::optional<Side> Map<htx_futures::json::Direction>::helper() const;

template <>
template <>
std::optional<UpdateType> Map<htx_futures::json::Event>::helper() const;

template <>
template <>
std::optional<PositionEffect> Map<htx_futures::json::Offset>::helper() const;

template <>
template <>
std::optional<OrderType> Map<htx_futures::json::OrderPriceType>::helper() const;

template <>
template <>
std::optional<Liquidity> Map<htx_futures::json::Role>::helper() const;

template <>
template <>
std::optional<OrderStatus> Map<std::int32_t>::helper() const;

// ===

template <>
template <>
std::optional<htx_futures::json::OrderPriceType> Map<roq::OrderType>::helper() const;

template <>
template <>
std::optional<htx_futures::json::Offset> Map<roq::PositionEffect>::helper() const;

template <>
template <>
std::optional<htx_futures::json::Direction> Map<roq::Side>::helper() const;

}  // namespace roq
