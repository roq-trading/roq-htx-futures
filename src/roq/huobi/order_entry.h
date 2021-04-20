/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/promise.h"

#include "roq/core/buffer.h"

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/web/client.h"

#include "roq/download.h"
#include "roq/server.h"

#include "roq/huobi/order_entry_state.h"
#include "roq/huobi/security.h"
#include "roq/huobi/shared.h"

#include "roq/huobi/json/account.h"
#include "roq/huobi/json/cancel_order.h"
#include "roq/huobi/json/exchange_info.h"
#include "roq/huobi/json/listen_key.h"
#include "roq/huobi/json/new_order.h"

namespace roq {
namespace huobi {

class OrderEntry final : public core::web::Client::Handler {
 public:
  struct ListenKeyUpdate final {
    std::string_view account;
    std::string_view listen_key;
  };

  struct SymbolsUpdate final {
    std::vector<std::string> &symbols;
  };

  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<ReferenceData> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<MarketStatus> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<FundsUpdate> &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(const ListenKeyUpdate &) = 0;
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  OrderEntry(Handler &, core::io::Context &, uint16_t stream_id, Security &, Shared &);

  OrderEntry(OrderEntry &&) = delete;
  OrderEntry(const OrderEntry &) = delete;

  bool ready() const;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  void operator()(
      const Event<CreateOrder> &, const std::string_view &request_id, uint32_t gateway_order_id);
  void operator()(
      const Event<ModifyOrder> &, const std::string_view &request_id, const server::OMS_Order &);
  void operator()(
      const Event<CancelOrder> &, const std::string_view &request_id, const server::OMS_Order &);

 protected:
  void operator()(const core::web::Client::Connected &);
  void operator()(const core::web::Client::Disconnected &);
  void operator()(const core::web::Client::Latency &);

  void operator()(ConnectionStatus);

  template <typename T>
  void get(std::function<void(const core::Promise<T> &)> &&);

  uint32_t download(OrderEntryState state);

  void download_listen_key();
  void download_account();
  void download_exchange_info();

  void refresh_listen_key();

  void create_order(
      const CreateOrder &,
      const std::string_view &cl_ord_id,
      std::function<void(const core::Promise<json::NewOrder> &)> &&);

  void cancel_order(
      const CancelOrder &,
      const std::string_view &request_id,
      const server::OMS_Order &,
      std::function<void(const core::Promise<json::CancelOrder> &)> &&);

  void operator()(const json::NewOrder &);
  void operator()(const json::CancelOrder &);

  void operator()(const json::ListenKey &);
  void operator()(const json::Account &);
  void operator()(const json::ExchangeInfo &);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  // connection
  core::web::Client connection_;
  // buffers
  core::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile exchange_info, account, listen_key, depth, new_order, cancel_order;
  } profile_;
  struct {
    core::metrics::Latency ping;
  } latency_;
  // security
  Security &security_;
  // cache
  Shared &shared_;
  absl::flat_hash_set<std::string> all_symbols_;
  std::string listen_key_;
  // state
  bool ready_ = false;
  std::chrono::nanoseconds listen_key_refresh_ = {};
  ConnectionStatus status_ = {};
  server::Download<OrderEntryState> download_;
};

}  // namespace huobi
}  // namespace roq
