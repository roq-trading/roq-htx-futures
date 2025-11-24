/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/json/map.hpp"

using namespace std::literals;

namespace roq {

namespace {
template <typename... Args>
using Helper = detail::MapHelper<Args...>;
}

// htx_futures ==> roq

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
    case ORDER_OPEN:
      return roq::UpdateType::INCREMENTAL;
    case ORDER_CANCEL:
      return roq::UpdateType::INCREMENTAL;
  }
  return {};
}

static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::UNDEFINED_INTERNAL}} == roq::UpdateType::UNDEFINED);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::INIT}} == roq::UpdateType::SNAPSHOT);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::SNAPSHOT}} == roq::UpdateType::SNAPSHOT);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::UPDATE}} == roq::UpdateType::INCREMENTAL);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::ORDER_OPEN}} == roq::UpdateType::INCREMENTAL);
static_assert(Helper{htx_futures::json::Event{htx_futures::json::Event::ORDER_CANCEL}} == roq::UpdateType::INCREMENTAL);

template <>
template <>
std::optional<roq::UpdateType> Map<htx_futures::json::Event>::helper() const {
  return Helper{args_};
}

// htx_futures::json::Side ==> roq::Side

template <>
template <>
constexpr Helper<htx_futures::json::Side>::operator std::optional<roq::Side>() const {
  switch (std::get<0>(args_)) {
    using enum htx_futures::json::Side::type_t;
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

static_assert(Helper{htx_futures::json::Side{htx_futures::json::Side::UNDEFINED_INTERNAL}} == roq::Side::UNDEFINED);
static_assert(Helper{htx_futures::json::Side{htx_futures::json::Side::BUY}} == roq::Side::BUY);
static_assert(Helper{htx_futures::json::Side{htx_futures::json::Side::SELL}} == roq::Side::SELL);

template <>
template <>
std::optional<roq::Side> Map<htx_futures::json::Side>::helper() const {
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

// roq::Side ==> htx_futures::json::Side

template <>
template <>
constexpr Helper<roq::Side>::operator std::optional<htx_futures::json::Side>() const {
  switch (std::get<0>(args_)) {
    using enum roq::Side;
    case UNDEFINED:
      return htx_futures::json::Side::UNDEFINED_INTERNAL;
    case BUY:
      return htx_futures::json::Side::BUY;
    case SELL:
      return htx_futures::json::Side::SELL;
  }
  return {};
}

static_assert(Helper{roq::Side::UNDEFINED} == htx_futures::json::Side{htx_futures::json::Side::UNDEFINED_INTERNAL});
static_assert(Helper{roq::Side::BUY} == htx_futures::json::Side{htx_futures::json::Side::BUY});
static_assert(Helper{roq::Side::SELL} == htx_futures::json::Side{htx_futures::json::Side::SELL});

template <>
template <>
std::optional<htx_futures::json::Side> Map<roq::Side>::helper() const {
  return Helper{args_};
}

}  // namespace roq
