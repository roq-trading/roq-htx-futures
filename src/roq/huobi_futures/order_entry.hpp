/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/buffer.hpp"
#include "roq/core/download.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/web/client.hpp"

#include "roq/server.hpp"

#include "roq/huobi_futures/order_entry_state.hpp"
#include "roq/huobi_futures/security.hpp"
#include "roq/huobi_futures/shared.hpp"

namespace roq {
namespace huobi_futures {

class OrderEntry final : public core::web::Client::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<ReferenceData> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<MarketStatus> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<FundsUpdate> &, bool is_last) = 0;
  };

  OrderEntry(Handler &, core::io::Context &, uint16_t stream_id, Security &, Shared &);

  OrderEntry(OrderEntry &&) = delete;
  OrderEntry(const OrderEntry &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  uint16_t operator()(
      const Event<CreateOrder> &, const oms::Order &, const std::string_view &request_id);
  uint16_t operator()(
      const Event<ModifyOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id);
  uint16_t operator()(
      const Event<CancelOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id);

  uint16_t operator()(const Event<CancelAllOrders> &, const std::string_view &request_id);

 protected:
  void operator()(const core::web::Client::Connected &);
  void operator()(const core::web::Client::Disconnected &);
  void operator()(const core::web::Client::Latency &);

  void operator()(ConnectionStatus);

  uint32_t download(OrderEntryState state);

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
    core::metrics::Profile listen_key, listen_key_ack,  //
        account, account_ack,                           //
        exchange_info, exchange_info_ack,               //
        new_order, new_order_ack,                       //
        cancel_order, cancel_order_ack;
  } profile_;
  struct {
    core::metrics::Latency ping;
  } latency_;
  // security
  Security &security_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};
  core::Download<OrderEntryState> download_;
};

}  // namespace huobi_futures
}  // namespace roq
