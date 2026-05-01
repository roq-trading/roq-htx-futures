/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/json/map.hpp"

using namespace std::literals;

namespace roq {

namespace {
template <typename... Args>
using Helper = detail::MapHelper<Args...>;
}

// htx_futures ==> roq

// htx_futures::json::Direction ==> roq::Side

template <>
template <>
constexpr Helper<htx_futures::json::Direction>::operator std::optional<roq::Side>() const {
  switch (std::get<0>(args_)) {
    using enum htx_futures::json::Direction::type_t;
    case UNDEFINED_INTERNAL:
      return roq::Side::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::Side::UNDEFINED;
    case BUY:
      return roq::Side::BUY;
    case SELL:
      return roq::Side::SELL;
  }
  return {};
}

static_assert(Helper{htx_futures::json::Direction{htx_futures::json::Direction::UNDEFINED_INTERNAL}} == roq::Side::UNDEFINED);
static_assert(Helper{htx_futures::json::Direction{htx_futures::json::Direction::BUY}} == roq::Side::BUY);
static_assert(Helper{htx_futures::json::Direction{htx_futures::json::Direction::SELL}} == roq::Side::SELL);

template <>
template <>
std::optional<roq::Side> Map<htx_futures::json::Direction>::helper() const {
  return Helper{args_};
}

// htx_futures::json::Event ==> roq::UpdateType

template <>
template <>
constexpr Helper<htx_futures::json::Event>::operator std::optional<roq::UpdateType>() const {
  switch (std::get<0>(args_)) {
    using enum htx_futures::json::Event::type_t;
    case UNDEFINED_INTERNAL:
      return roq::UpdateType::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::UpdateType::UNDEFINED;
    case INIT:
      return roq::UpdateType::SNAPSHOT;
    case SNAPSHOT:
      return roq::UpdateType::SNAPSHOT;
    case UPDATE:
      return roq::UpdateType::INCREMENTAL;
    case SETTLEMENT:
      return roq::UpdateType::INCREMENTAL;
    case ORDER_OPEN:
      return roq::UpdateType::INCREMENTAL;
    case ORDER_CANCEL:
      return roq::UpdateType::INCREMENTAL;
    case ORDER_MATCH:
      return roq::UpdateType::INCREMENTAL;
    case ORDER_CLOSE:
      return roq::UpdateType::INCREMENTAL;
    case CONTRACT_SYSTEM:
      return roq::UpdateType::INCREMENTAL;
  }
  return {};
}

static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::UNDEFINED_INTERNAL}} == roq::UpdateType::UNDEFINED);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::INIT}} == roq::UpdateType::SNAPSHOT);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::SNAPSHOT}} == roq::UpdateType::SNAPSHOT);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::UPDATE}} == roq::UpdateType::INCREMENTAL);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::SETTLEMENT}} == roq::UpdateType::INCREMENTAL);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::ORDER_OPEN}} == roq::UpdateType::INCREMENTAL);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::ORDER_CANCEL}} == roq::UpdateType::INCREMENTAL);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::ORDER_MATCH}} == roq::UpdateType::INCREMENTAL);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::ORDER_CLOSE}} == roq::UpdateType::INCREMENTAL);

template <>
template <>
std::optional<roq::UpdateType> Map<htx_futures::json::Event>::helper() const {
  return Helper{args_};
}

// htx_futures::json::Offset ==> roq::PositionEffect

template <>
template <>
constexpr Helper<htx_futures::json::Offset>::operator std::optional<roq::PositionEffect>() const {
  switch (std::get<0>(args_)) {
    using enum htx_futures::json::Offset::type_t;
    case UNDEFINED_INTERNAL:
      return roq::PositionEffect::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::PositionEffect::UNDEFINED;
    case OPEN:
      return roq::PositionEffect::OPEN;
    case CLOSE:
      return roq::PositionEffect::CLOSE;
    case BOTH:
      return roq::PositionEffect::UNDEFINED;
  }
  return {};
}

static_assert(Helper{htx_futures::json::Offset{htx_futures::json::Offset::UNDEFINED_INTERNAL}} == roq::PositionEffect::UNDEFINED);
static_assert(Helper{htx_futures::json::Offset{htx_futures::json::Offset::OPEN}} == roq::PositionEffect::OPEN);
static_assert(Helper{htx_futures::json::Offset{htx_futures::json::Offset::CLOSE}} == roq::PositionEffect::CLOSE);
static_assert(Helper{htx_futures::json::Offset{htx_futures::json::Offset::BOTH}} == roq::PositionEffect::UNDEFINED);

template <>
template <>
std::optional<roq::PositionEffect> Map<htx_futures::json::Offset>::helper() const {
  return Helper{args_};
}

// htx_futures::json::OrderPriceType ==> roq::OrderType

template <>
template <>
constexpr Helper<htx_futures::json::OrderPriceType>::operator std::optional<roq::OrderType>() const {
  switch (std::get<0>(args_)) {
    using enum htx_futures::json::OrderPriceType::type_t;
    case UNDEFINED_INTERNAL:
      return roq::OrderType::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::OrderType::UNDEFINED;
    case LIMIT:
      return roq::OrderType::LIMIT;
    case OPPONENT:
      return roq::OrderType::UNDEFINED;
    case POST_ONLY:
      return roq::OrderType::LIMIT;
    case OPTIMAL_5:
      return roq::OrderType::UNDEFINED;
    case OPTIMAL_10:
      return roq::OrderType::UNDEFINED;
    case OPTIMAL_20:
      return roq::OrderType::UNDEFINED;
    case IOC:
      return roq::OrderType::LIMIT;
    case FOK:
      return roq::OrderType::LIMIT;
    case OPPONENT_IOC:
      return roq::OrderType::UNDEFINED;
    case OPTIMAL_5_IOC:
      return roq::OrderType::UNDEFINED;
    case OPTIMAL_10_IOC:
      return roq::OrderType::UNDEFINED;
    case OPTIMAL_20_IOC:
      return roq::OrderType::UNDEFINED;
    case OPPONENT_FOK:
      return roq::OrderType::UNDEFINED;
    case OPTIMAL_5_FOK:
      return roq::OrderType::UNDEFINED;
    case OPTIMAL_10_FOK:
      return roq::OrderType::UNDEFINED;
    case OPTIMAL_20_FOK:
      return roq::OrderType::UNDEFINED;
  }
  return {};
}

static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::UNDEFINED_INTERNAL}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPPONENT}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::POST_ONLY}} == roq::OrderType::LIMIT);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPTIMAL_5}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPTIMAL_10}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPTIMAL_20}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::IOC}} == roq::OrderType::LIMIT);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::FOK}} == roq::OrderType::LIMIT);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPPONENT_IOC}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPTIMAL_5_IOC}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPTIMAL_10_IOC}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPTIMAL_20_IOC}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPPONENT_FOK}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPTIMAL_5_FOK}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPTIMAL_10_FOK}} == roq::OrderType::UNDEFINED);
static_assert(Helper{htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::OPTIMAL_20_FOK}} == roq::OrderType::UNDEFINED);

template <>
template <>
std::optional<roq::OrderType> Map<htx_futures::json::OrderPriceType>::helper() const {
  return Helper{args_};
}

// htx_futures::json::Role ==> roq::Liquidity

template <>
template <>
constexpr Helper<htx_futures::json::Role>::operator std::optional<roq::Liquidity>() const {
  switch (std::get<0>(args_)) {
    using enum htx_futures::json::Role::type_t;
    case UNDEFINED_INTERNAL:
      return roq::Liquidity::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::Liquidity::UNDEFINED;
    case MAKER:
      return roq::Liquidity::MAKER;
    case TAKER:
      return roq::Liquidity::TAKER;
  }
  return {};
}

static_assert(Helper{htx_futures::json::Role{htx_futures::json::Role::UNDEFINED_INTERNAL}} == roq::Liquidity::UNDEFINED);
static_assert(Helper{htx_futures::json::Role{htx_futures::json::Role::MAKER}} == roq::Liquidity::MAKER);
static_assert(Helper{htx_futures::json::Role{htx_futures::json::Role::TAKER}} == roq::Liquidity::TAKER);

template <>
template <>
std::optional<roq::Liquidity> Map<htx_futures::json::Role>::helper() const {
  return Helper{args_};
}

// std::int32_t ==> roq::OrderStatus

template <>
template <>
constexpr Helper<std::int32_t>::operator std::optional<roq::OrderStatus>() const {
  switch (std::get<0>(args_)) {
    case 1:  // Ready to submit the orders;
    case 2:  // Ready to submit the orders;
      return roq::OrderStatus::ACCEPTED;
    case 3:  // Have sumbmitted the orders;
    case 4:  // Orders partially matched;
      return roq::OrderStatus::WORKING;
    case 5:  // Orders cancelled with partially matched;
      return roq::OrderStatus::CANCELED;
    case 6:  // Orders fully matched;
      return roq::OrderStatus::COMPLETED;
    case 7:  // Orders cancelled with partially matched;
      return roq::OrderStatus::CANCELED;
  }
  return {};
}

static_assert(Helper{std::int32_t{1}} == roq::OrderStatus::ACCEPTED);
static_assert(Helper{std::int32_t{2}} == roq::OrderStatus::ACCEPTED);
static_assert(Helper{std::int32_t{3}} == roq::OrderStatus::WORKING);
static_assert(Helper{std::int32_t{4}} == roq::OrderStatus::WORKING);
static_assert(Helper{std::int32_t{5}} == roq::OrderStatus::CANCELED);
static_assert(Helper{std::int32_t{6}} == roq::OrderStatus::COMPLETED);
static_assert(Helper{std::int32_t{7}} == roq::OrderStatus::CANCELED);

template <>
template <>
std::optional<roq::OrderStatus> Map<std::int32_t>::helper() const {
  return Helper{args_};
}
// roq ==> htx_futures::json

// roq::OrderType ==> htx_futures::json::OrderPriceType

template <>
template <>
constexpr Helper<roq::OrderType>::operator std::optional<htx_futures::json::OrderPriceType>() const {
  switch (std::get<0>(args_)) {
    using enum roq::OrderType;
    case UNDEFINED:
      return htx_futures::json::OrderPriceType::UNDEFINED_INTERNAL;
    case MARKET:
      return htx_futures::json::OrderPriceType::FOK;  // XXX FIXME TODO ???
    case LIMIT:
      return htx_futures::json::OrderPriceType::LIMIT;
  }
  return {};
}

static_assert(Helper{roq::OrderType::UNDEFINED} == htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::UNDEFINED_INTERNAL});
static_assert(Helper{roq::OrderType::MARKET} == htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::FOK});
static_assert(Helper{roq::OrderType::LIMIT} == htx_futures::json::OrderPriceType{htx_futures::json::OrderPriceType::LIMIT});

template <>
template <>
std::optional<htx_futures::json::OrderPriceType> Map<roq::OrderType>::helper() const {
  return Helper{args_};
}

// roq::PositionEffect ==> htx_futures::json::Offset

template <>
template <>
constexpr Helper<roq::PositionEffect>::operator std::optional<htx_futures::json::Offset>() const {
  switch (std::get<0>(args_)) {
    using enum roq::PositionEffect;
    case UNDEFINED:
      return htx_futures::json::Offset::UNDEFINED_INTERNAL;
    case OPEN:
      return htx_futures::json::Offset::OPEN;
    case CLOSE:
      return htx_futures::json::Offset::CLOSE;
  }
  return {};
}

static_assert(Helper{roq::PositionEffect::UNDEFINED} == htx_futures::json::Offset{htx_futures::json::Offset::UNDEFINED_INTERNAL});
static_assert(Helper{roq::PositionEffect::OPEN} == htx_futures::json::Offset{htx_futures::json::Offset::OPEN});
static_assert(Helper{roq::PositionEffect::CLOSE} == htx_futures::json::Offset{htx_futures::json::Offset::CLOSE});

template <>
template <>
std::optional<htx_futures::json::Offset> Map<roq::PositionEffect>::helper() const {
  return Helper{args_};
}

// roq::Side ==> htx_futures::json::Direction

template <>
template <>
constexpr Helper<roq::Side>::operator std::optional<htx_futures::json::Direction>() const {
  switch (std::get<0>(args_)) {
    using enum roq::Side;
    case UNDEFINED:
      return htx_futures::json::Direction::UNDEFINED_INTERNAL;
    case BUY:
      return htx_futures::json::Direction::BUY;
    case SELL:
      return htx_futures::json::Direction::SELL;
  }
  return {};
}

static_assert(Helper{roq::Side::UNDEFINED} == htx_futures::json::Direction{htx_futures::json::Direction::UNDEFINED_INTERNAL});
static_assert(Helper{roq::Side::BUY} == htx_futures::json::Direction{htx_futures::json::Direction::BUY});
static_assert(Helper{roq::Side::SELL} == htx_futures::json::Direction{htx_futures::json::Direction::SELL});

template <>
template <>
std::optional<htx_futures::json::Direction> Map<roq::Side>::helper() const {
  return Helper{args_};
}

}  // namespace roq
