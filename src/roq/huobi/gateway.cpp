/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi/gateway.h"

#include <algorithm>
#include <cctype>
#include <limits>

#include "roq/logging.h"

#include "roq/core/string_utils/string_builder.h"

#include "roq/core/charconv.h"
#include "roq/core/clock.h"
#include "roq/core/utils.h"

#include "roq/huobi/flags.h"

#include "roq/huobi/json/utils.h"

using namespace roq::literals;

namespace roq {
namespace huobi {

namespace {
static auto create_security(const Config &config) {
  absl::flat_hash_map<std::string, std::unique_ptr<Security>> result;
  for (auto &[_, iter] : config.accounts) {
    result.try_emplace(iter.name, std::make_unique<Security>(config, iter.name));
  }
  return result;
}

template <typename T>
static auto create_order_entry(
    Gateway &gateway,
    core::io::Context &context,
    uint16_t &stream_id,
    T &security,
    Shared &shared) {
  absl::flat_hash_map<std::string, std::unique_ptr<OrderEntry>> result;
  for (auto &iter : security) {
    result.try_emplace(
        iter.first,
        std::make_unique<OrderEntry>(gateway, context, ++stream_id, *iter.second, shared));
  }
  return result;
}

template <typename T>
static auto create_drop_copy(T &security) {
  absl::flat_hash_map<std::string, std::unique_ptr<DropCopy>> result;
  for (auto &iter : security) {
    result.try_emplace(iter.first, nullptr);
  }
  return result;
}
}  // namespace

Gateway::Gateway(server::Dispatcher &dispatcher, const Config &config)
    : dispatcher_(dispatcher), master_account_(config.get_master_account()),
      security_(create_security(config)), shared_(dispatcher),
      order_entry_(create_order_entry(*this, context_, stream_id_, security_, shared_)),
      drop_copy_(create_drop_copy(security_)) {
  if (ROQ_UNLIKELY(Flags::rest_cancel_on_disconnect()))
    log::fatal("Exchange does *NOT* support cancel on disconnect"_sv);
}

void Gateway::operator()(const Event<Start> &event) {
  log::info("Starting the gateway..."_sv);
  for (auto &[_, order_entry] : order_entry_)
    (*order_entry)(event);
  for (auto &[_, drop_copy] : drop_copy_)
    if (static_cast<bool>(drop_copy))
      (*drop_copy)(event);
  assert(market_data_.empty());
  // order_entry_.download.begin();
}

void Gateway::operator()(const Event<Stop> &event) {
  log::info("Stopping the gateway..."_sv);
  for (auto &iter : market_data_)
    (*iter)(event);
  for (auto &[_, drop_copy] : drop_copy_)
    if (static_cast<bool>(drop_copy))
      (*drop_copy)(event);
  for (auto &[_, order_entry] : order_entry_)
    (*order_entry)(event);
}

void Gateway::operator()(const Event<Timer> &event) {
  for (auto &[_, order_entry] : order_entry_)
    (*order_entry)(event);
  for (auto &[_, drop_copy] : drop_copy_)
    if (static_cast<bool>(drop_copy))
      (*drop_copy)(event);
  for (auto &iter : market_data_)
    (*iter)(event);
  context_.dispatch(true);
}

void Gateway::operator()(const Event<Connected> &) {
}

void Gateway::operator()(const Event<Disconnected> &) {
}

void Gateway::operator()(const server::Trace<StreamStatus> &event) {
  dispatcher_(event);
}

void Gateway::operator()(const server::Trace<ExternalLatency> &event) {
  dispatcher_(event);
}

void Gateway::operator()(const server::Trace<ReferenceData> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<MarketStatus> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<TopOfBook> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<MarketByPriceUpdate> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<TradeSummary> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<StatisticsUpdate> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const server::Trace<FundsUpdate> &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Gateway::operator()(const OrderEntry::ListenKeyUpdate &listen_key_update) {
  auto &account = listen_key_update.account;
  assert(!account.empty());
  auto iter = drop_copy_.find(account);
  if (iter == drop_copy_.end()) {
    log::fatal(R"(Unexpected: account="{}")"_fmt, account);
  } else if (!static_cast<bool>((*iter).second)) {
    log::info(R"(Create drop-copy (user-stream) for account="{}")"_fmt, account);
    auto drop_copy = std::make_unique<DropCopy>(
        *this, context_, ++stream_id_, *security_[account], shared_, listen_key_update.listen_key);
    MessageInfo message_info;  // XXX something sensible
    Start start;
    create_event_and_dispatch(*drop_copy, message_info, start);
    (*iter).second = std::move(drop_copy);
  }
}

void Gateway::operator()(OrderEntry::SymbolsUpdate &symbols_update) {
  auto &symbols = symbols_update.symbols;
  for (auto &iter : market_data_) {
    if (symbols.empty())
      break;
    (*iter).update_subscriptions(symbols);
  }
  for (;;) {
    if (symbols.empty())
      break;
    log::info("Create market-data (user-stream)"_sv);
    auto market_data = std::make_unique<MarketData>(*this, context_, ++stream_id_, shared_);
    (*market_data).update_subscriptions(symbols);
    MessageInfo message_info;  // XXX something sensible
    Start start;
    create_event_and_dispatch(*market_data, message_info, start);
    market_data_.emplace_back(std::move(market_data));
  }
}

void Gateway::operator()(
    const Event<CreateOrder> &event,
    const std::string_view &request_id,
    uint32_t gateway_order_id) {
  assert(!event.value.account.empty());
  get_order_entry(event.value.account)(event, request_id, gateway_order_id);
}

void Gateway::operator()(
    const Event<ModifyOrder> &event,
    const std::string_view &request_id,
    const server::OMS_Order &order) {
  assert(!event.value.account.empty());
  assert(event.value.account == order.account);
  get_order_entry(event.value.account)(event, request_id, order);
}

void Gateway::operator()(
    const Event<CancelOrder> &event,
    const std::string_view &request_id,
    const server::OMS_Order &order) {
  assert(!event.value.account.empty());
  assert(event.value.account == order.account);
  get_order_entry(event.value.account)(event, request_id, order);
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

OrderEntry &Gateway::get_order_entry(const std::string_view &account) {
  auto iter = order_entry_.find(account);
  if (iter != order_entry_.end())
    return *(*iter).second;
  throw RuntimeErrorException(R"(Unknown account="{}")"_fmt, account);
}

}  // namespace huobi
}  // namespace roq
