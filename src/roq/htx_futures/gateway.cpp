/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/gateway.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

#include "roq/logging.hpp"

#include "roq/clock.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/htx_futures/order_entry_rest.hpp"
#include "roq/htx_futures/order_entry_ws.hpp"

#include "roq/htx_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {

// === HELPERS ===

namespace {
template <typename R>
R create_accounts(auto &config, auto &settings) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, account] : config.accounts) {
    result.try_emplace(static_cast<std::string_view>(account.name), std::make_unique<Account>(config, account.name, settings.ws.order_uri));
  }
  return result;
}

template <typename R>
R create_order_entry_rest(Gateway &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared, auto has_real_accounts) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  if (has_real_accounts) {
    for (auto &[name, account] : accounts) {
      result.try_emplace(static_cast<std::string_view>(name), std::make_unique<OrderEntryREST>(gateway, context, ++stream_id, *account, shared));
    }
  }
  return result;
}

template <typename R>
R create_order_entry_ws(Gateway &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared, auto has_real_accounts) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  if (shared.settings.ws_api && has_real_accounts) {
    for (auto &[name, account] : accounts) {
      result.try_emplace(static_cast<std::string_view>(name), std::make_unique<OrderEntryWS>(gateway, context, ++stream_id, *account, shared));
    }
  }
  return result;
}

template <typename R>
R create_drop_copy(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[name, account] : accounts) {
    result.try_emplace(static_cast<std::string_view>(name), std::make_unique<DropCopy>(gateway, context, ++stream_id, *account, shared));
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Gateway::Gateway(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context)
    : dispatcher_{dispatcher}, accounts_{create_accounts<decltype(accounts_)>(config, settings)}, context_{context}, shared_{dispatcher, settings},
      rest_{*this, context_, ++stream_id_, shared_},
      order_entry_rest_{create_order_entry_rest<decltype(order_entry_rest_)>(*this, context_, stream_id_, accounts_, shared_, !std::empty(config.accounts))},
      order_entry_ws_{create_order_entry_ws<decltype(order_entry_ws_)>(*this, context_, stream_id_, accounts_, shared_, !std::empty(config.accounts))},
      drop_copy_{create_drop_copy<decltype(drop_copy_)>(*this, context_, stream_id_, accounts_, shared_)} {
  if (settings.rest.cancel_on_disconnect) {
    log::fatal("Exchange does *NOT* support cancel on disconnect"sv);
  }
}

void Gateway::operator()(Event<Start> const &event) {
  log::info("Starting..."sv);
  assert(std::empty(market_data_));
  assert(std::empty(web_socket_));
  assert(std::empty(web_socket_2_));
  dispatch(event);
}

void Gateway::operator()(Event<Stop> const &event) {
  log::info("Stopping..."sv);
  dispatch(event);
}

void Gateway::operator()(Event<Timer> const &event) {
  dispatch(event);
}

void Gateway::operator()(Event<Control> const &event) {
  auto &[message_info, control] = event;
  switch (control.action) {
    using enum Action;
    case UNDEFINED:
      assert(false);
      break;
    case ENABLE:
      dispatcher_(State::ENABLED);
      break;
    case DISABLE:
      dispatcher_(State::DISABLED);
      break;
  }
}

void Gateway::operator()(Event<Connected> const &) {
}

void Gateway::operator()(Event<Disconnected> const &) {
}

void Gateway::operator()(Trace<StreamStatus> const &event) {
  dispatcher_(event);
}

void Gateway::operator()(Trace<ExternalLatency> const &event) {
  dispatcher_(event);
}

void Gateway::operator()(Trace<ReferenceData> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<MarketStatus> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<TopOfBook> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<MarketByPriceUpdate> const &event, bool is_last) {
  auto callback = []([[maybe_unused]] auto &market_by_price) {};
  dispatcher_(event, is_last, bids_, asks_, callback);
}

void Gateway::operator()(Trace<TradeSummary> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<StatisticsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<FundsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Trace<PositionUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(Rest::SymbolsUpdate &symbols_update) {
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(size);
  for (auto &iter : market_data_) {
    (*iter).subscribe(start_from);
  }
  for (auto &iter : web_socket_) {
    (*iter).subscribe(start_from);
  }
  for (auto &iter : web_socket_2_) {
    (*iter).subscribe(start_from);
  }
}

void Gateway::ensure_symbol_slices(size_t size) {
  // market data
  while (std::size(market_data_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(market_data_);
    log::debug("Create MarketData (stream_id={}, index={})"sv, stream_id, index);
    auto market_data = std::make_unique<MarketData>(*this, context_, stream_id, shared_, index);
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
    auto web_socket = std::make_unique<WebSocket>(*this, context_, stream_id, shared_, index);
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
    auto web_socket_2 = std::make_unique<WebSocket2>(*this, context_, stream_id, shared_, index);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*web_socket_2, message_info, start);
    web_socket_2_.emplace_back(std::move(web_socket_2));
  }
}

uint16_t Gateway::operator()(Event<CreateOrder> const &event, server::oms::Order const &order, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, order, request_id);
}

uint16_t Gateway::operator()(
    Event<ModifyOrder> const &event, server::oms::Order const &order, std::string_view const &request_id, std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, request_id, previous_request_id);
}

uint16_t Gateway::operator()(
    Event<CancelOrder> const &event, server::oms::Order const &order, std::string_view const &request_id, std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, request_id, previous_request_id);
}

uint16_t Gateway::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, request_id);
}

uint16_t Gateway::operator()(Event<MassQuote> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Gateway::operator()(Event<CancelQuotes> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

void Gateway::operator()(metrics::Writer &writer) const {
  dispatch_helper(*this, writer);
}

template <typename... Args>
void Gateway::dispatch(Args &&...args) {
  dispatch_helper(*this, std::forward<Args>(args)...);
}

template <typename... Args>
void Gateway::dispatch_helper(auto &self, Args &&...args) {
  auto helper = [&](auto &target) { target(std::forward<Args>(args)...); };
  helper(self.rest_);
  for (auto &[_, item] : self.order_entry_rest_) {
    helper(*item);
  }
  for (auto &[_, item] : self.order_entry_ws_) {
    helper(*item);
  }
  for (auto &[_, item] : self.drop_copy_) {
    if (static_cast<bool>(item)) {
      helper(*item);
    }
  }
  for (auto &item : self.market_data_) {
    helper(*item);
  }
  for (auto &item : self.web_socket_) {
    helper(*item);
  }
  for (auto &item : self.web_socket_2_) {
    helper(*item);
  }
}

OrderEntry &Gateway::get_order_entry(std::string_view const &account) {
  if (shared_.settings.ws_api) {
    auto iter = order_entry_ws_.find(account);
    if (iter != std::end(order_entry_ws_)) {
      return *(*iter).second;
    }
  } else {
    auto iter = order_entry_rest_.find(account);
    if (iter != std::end(order_entry_rest_)) {
      return *(*iter).second;
    }
  }
  throw RuntimeError{R"(Unknown account="{}")"sv, account};
}

}  // namespace htx_futures
}  // namespace roq
