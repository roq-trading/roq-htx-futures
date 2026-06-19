/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/gateway/controller.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

#include "roq/logging.hpp"

#include "roq/clock.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/htx_futures/gateway/api.hpp"

#include "roq/htx_futures/gateway/order_entry_rest.hpp"
#include "roq/htx_futures/gateway/order_entry_ws.hpp"

#include "roq/htx_futures/protocol/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace gateway {

// === CONSTANTS ===

namespace {
uint8_t const API_USDT_M_FUTURES = 0x1;
uint8_t const API_COIN_M_DELIVERY = 0x2;
uint8_t const API_COIN_M_PERPETUAL = 0x3;
}  // namespace

// === HELPERS ===

namespace {
template <typename R>
R create_accounts(auto &config, auto &settings, auto &api) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, account] : config.accounts) {
    auto margin_mode = [&]() -> MarginMode {
      if (account.margin_mode != MarginMode{}) {
        return account.margin_mode;
      }
      if (settings.margin_mode != MarginMode{}) {
        return settings.margin_mode;
      }
      return api.order_management.default_margin_mode;
    }();
    switch (margin_mode) {
      using enum MarginMode;
      case UNDEFINED:
        log::fatal(R"(Unexpected: no margin_mode for account name="{}")"sv, account.name);
      case ISOLATED:
      case CROSS:
        break;
      case PORTFOLIO:
        log::fatal("Unexpected: exchange does not support margin_mode={}"sv, margin_mode);
    }
    result.try_emplace(static_cast<std::string_view>(account.name), std::make_unique<Account>(config, account.name, margin_mode, settings.ws.order_uri));
  }
  return result;
}

template <typename R>
R create_order_entry_rest(Controller &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared, auto has_real_accounts) {
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
R create_order_entry_ws(Controller &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared, auto has_real_accounts) {
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

std::unique_ptr<server::Handler> Controller::create(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context) {
  return std::make_unique<Controller>(dispatcher, settings, config, context);
}

uint8_t Controller::parse_api(Settings const &settings) {
  auto api = API::parse_api(settings);
  switch (api) {
    using enum gateway::API::Key;
    case USDT_M_FUTURES:
      return API_USDT_M_FUTURES;
    case COIN_M_DELIVERY:
      return API_COIN_M_DELIVERY;
    case COIN_M_PERPETUAL:
      return API_COIN_M_PERPETUAL;
  }
  log::fatal(R"(Unexpected: api="{}")"sv, settings.app.api);
}

Controller::Controller(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context)
    : dispatcher_{dispatcher}, context_{context}, shared_{dispatcher, settings}, accounts_{create_accounts<decltype(accounts_)>(config, settings, shared_.api)},
      rest_{*this, context_, ++stream_id_, shared_},
      order_entry_rest_{create_order_entry_rest<decltype(order_entry_rest_)>(*this, context_, stream_id_, accounts_, shared_, !std::empty(config.accounts))},
      order_entry_ws_{create_order_entry_ws<decltype(order_entry_ws_)>(*this, context_, stream_id_, accounts_, shared_, !std::empty(config.accounts))},
      drop_copy_{create_drop_copy<decltype(drop_copy_)>(*this, context_, stream_id_, accounts_, shared_)} {
  if (settings.rest.cancel_on_disconnect) {
    log::fatal("Exchange does *NOT* support cancel on disconnect"sv);
  }
}

// server::Handler

void Controller::operator()(Event<Start> const &event) {
  log::info("Starting..."sv);
  assert(std::empty(market_data_));
  assert(std::empty(web_socket_));
  assert(std::empty(web_socket_2_));
  dispatch(event);
}

void Controller::operator()(Event<Stop> const &event) {
  log::info("Stopping..."sv);
  dispatch(event);
}

void Controller::operator()(Event<Timer> const &event) {
  dispatch(event);
}

void Controller::operator()(Event<Control> const &event) {
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

void Controller::operator()(Event<Connected> const &) {
}

void Controller::operator()(Event<Disconnected> const &) {
}

void Controller::operator()(Event<Subscribe> const &event) {
  auto &[message_info, subscribe] = event;
  std::vector<Symbol> symbols;
  for (auto &item : subscribe.symbols) {
    if (shared_.all_symbols.emplace(item).second) {
      symbols.emplace_back(item);
    } else {
      log::warn(R"(*** DUPLICATE SUBSCRIPTION *** (symbol="{}")"sv, item);
    }
  }
  auto symbols_update = Rest::SymbolsUpdate{
      .symbols = symbols,
  };
  (*this)(symbols_update);
}

uint16_t Controller::operator()(
    Event<CreateOrder> const &event, server::oms::Order const &order, server::oms::RefData const &ref_data, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, order, ref_data, request_id);
}

uint16_t Controller::operator()(
    Event<ModifyOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, ref_data, request_id, previous_request_id);
}

uint16_t Controller::operator()(
    Event<CancelOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  return get_order_entry(event.value.account)(event, order, ref_data, request_id, previous_request_id);
}

uint16_t Controller::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  return get_order_entry(event.value.account)(event, request_id);
}

uint16_t Controller::operator()(Event<MassQuote> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(Event<CancelQuotes> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

void Controller::operator()(metrics::Writer &writer) const {
  dispatch_helper(*this, writer);
}

// Rest::Handler

void Controller::operator()(Rest::SymbolsUpdate &symbols_update) {
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

// utilities

void Controller::ensure_symbol_slices(size_t size) {
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

template <typename... Args>
void Controller::dispatch(Args &&...args) {
  dispatch_helper(*this, std::forward<Args>(args)...);
}

template <typename... Args>
void Controller::dispatch_helper(auto &self, Args &&...args) {
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

OrderEntry &Controller::get_order_entry(std::string_view const &account) {
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

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
