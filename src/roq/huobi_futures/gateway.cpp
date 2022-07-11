/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/gateway.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

#include "roq/logging.hpp"

#include "roq/core/charconv.hpp"
#include "roq/core/clock.hpp"
#include "roq/core/utils.hpp"

#include "roq/io/event/context_factory.hpp"

#include "roq/huobi_futures/flags.hpp"

#include "roq/huobi_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

namespace {
template <typename R>
auto create_security(Config const &config) {
  R result;
  for (auto &[_, iter] : config.accounts)
    result.try_emplace(iter.name, std::make_unique<Security>(config, iter.name));
  return result;
}

template <typename R, typename T>
auto create_order_entry(
    Gateway &gateway,
    io::Context &context,
    uint16_t &stream_id,
    T &security,
    Shared &shared,
    bool has_real_accounts) {
  R result;
  if (has_real_accounts) {
    for (auto &iter : security)
      result.try_emplace(
          iter.first, std::make_unique<OrderEntry>(gateway, context, ++stream_id, *(iter.second), shared));
  }
  return result;
}

template <typename R, typename T>
auto create_drop_copy(Gateway &gateway, io::Context &context, uint16_t &stream_id, T &security, Shared &shared) {
  R result;
  for (auto &iter : security)
    result.try_emplace(iter.first, std::make_unique<DropCopy>(gateway, context, ++stream_id, *(iter.second), shared));
  return result;
}
}  // namespace

Gateway::Gateway(server::Dispatcher &dispatcher, Config const &config)
    : dispatcher_(dispatcher), master_account_(config.get_master_account()),
      security_(create_security<decltype(security_)>(config)), context_(io::event::ContextFactory::create()),
      shared_(dispatcher), rest_(*this, *context_, ++stream_id_, shared_),
      order_entry_(create_order_entry<decltype(order_entry_)>(
          *this, *context_, stream_id_, security_, shared_, !std::empty(config.accounts))),
      drop_copy_(create_drop_copy<decltype(drop_copy_)>(*this, *context_, stream_id_, security_, shared_)) {
  if (Flags::rest_cancel_on_disconnect())
    log::fatal("Exchange does *NOT* support cancel on disconnect"sv);
}

void Gateway::operator()(Event<Start> const &event) {
  log::info("Starting the gateway..."sv);
  rest_(event);
  for (auto &[_, order_entry] : order_entry_)
    (*order_entry)(event);
  for (auto &[_, drop_copy] : drop_copy_)
    if (static_cast<bool>(drop_copy))
      (*drop_copy)(event);
  assert(std::empty(market_data_));
  assert(std::empty(web_socket_));
  assert(std::empty(web_socket_2_));
}

void Gateway::operator()(Event<Stop> const &event) {
  log::info("Stopping the gateway..."sv);
  for (auto &iter : web_socket_2_)
    (*iter)(event);
  for (auto &iter : web_socket_)
    (*iter)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
  for (auto &[_, drop_copy] : drop_copy_)
    if (static_cast<bool>(drop_copy))
      (*drop_copy)(event);
  for (auto &[_, order_entry] : order_entry_)
    (*order_entry)(event);
  rest_(event);
}

void Gateway::operator()(Event<Timer> const &event) {
  rest_(event);
  for (auto &[_, order_entry] : order_entry_)
    (*order_entry)(event);
  for (auto &[_, drop_copy] : drop_copy_)
    if (static_cast<bool>(drop_copy))
      (*drop_copy)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
  for (auto &iter : web_socket_)
    (*iter)(event);
  for (auto &iter : web_socket_2_)
    (*iter)(event);
  (*context_).drain();
}

void Gateway::operator()(Event<Connected> const &) {
}

void Gateway::operator()(Event<Disconnected> const &event) {
  auto const &[message_info, disconnected] = event;
  if (disconnected.order_cancel_policy != OrderCancelPolicy{}) {
    log::warn("*** CANCEL-ON-DISCONNECT *NOT* SUPPORTED ***"sv);
  }
}

void Gateway::operator()(Trace<StreamStatus const> const &event) {
  dispatcher_(event);
}

void Gateway::operator()(Trace<ExternalLatency const> const &event) {
  dispatcher_(event);
}

void Gateway::operator()(Trace<ReferenceData const> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<MarketStatus const> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<TopOfBook const> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<MarketByPriceUpdate const> const &event, bool is_last, bool refresh) {
  dispatcher_(
      event, is_last, refresh, shared_.final_bids, shared_.final_asks, []([[maybe_unused]] auto &market_by_price) {});
}

void Gateway::operator()(Trace<TradeSummary const> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<StatisticsUpdate const> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<FundsUpdate const> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Rest::SymbolsUpdate &symbols_update) {
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(size);
  for (auto &iter : market_data_)
    (*iter).subscribe(start_from);
  for (auto &iter : web_socket_)
    (*iter).subscribe(start_from);
  for (auto &iter : web_socket_2_)
    (*iter).subscribe(start_from);
}

void Gateway::ensure_symbol_slices(size_t size) {
  // market data
  while (std::size(market_data_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(market_data_);
    log::debug("Create MarketData (stream_id={}, index={})"sv, stream_id, index);
    auto market_data = std::make_unique<MarketData>(*this, *context_, stream_id, shared_, index);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*market_data, message_info, start);
    market_data_.emplace_back(std::move(market_data));
  }
  // web socket #1
  while (std::size(web_socket_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(web_socket_);
    log::debug("Create WebSocket #1 (stream_id={}, index={})"sv, stream_id, index);
    auto web_socket = std::make_unique<WebSocket>(*this, *context_, stream_id, shared_, index);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*web_socket, message_info, start);
    web_socket_.emplace_back(std::move(web_socket));
  }
  // web socket #2
  while (std::size(web_socket_2_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(web_socket_2_);
    log::debug("Create WebSocket #2 (stream_id={}, index={})"sv, stream_id, index);
    auto web_socket_2 = std::make_unique<WebSocket2>(*this, *context_, stream_id, shared_, index);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*web_socket_2, message_info, start);
    web_socket_2_.emplace_back(std::move(web_socket_2));
  }
}

uint16_t Gateway::operator()(
    Event<CreateOrder> const &event, oms::Order const &order, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, order, request_id);
}

uint16_t Gateway::operator()(
    Event<ModifyOrder> const &event,
    oms::Order const &order,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, request_id, previous_request_id);
}

uint16_t Gateway::operator()(
    Event<CancelOrder> const &event,
    oms::Order const &order,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, request_id, previous_request_id);
}

uint16_t Gateway::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, request_id);
}

void Gateway::operator()(metrics::Writer &writer) {
  for (auto &[_, order_entry] : order_entry_)
    (*order_entry)(writer);
  for (auto &[_, drop_copy] : drop_copy_)
    if (static_cast<bool>(drop_copy))
      (*drop_copy)(writer);
  for (auto &iter : market_data_)
    (*iter)(writer);
}

OrderEntry &Gateway::get_order_entry(std::string_view const &account) {
  auto iter = order_entry_.find(account);
  if (iter != std::end(order_entry_))
    return *(*iter).second;
  throw RuntimeError(R"(Unknown account="{}")"sv, account);
}

}  // namespace huobi_futures
}  // namespace roq
