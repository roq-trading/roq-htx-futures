/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/server/stream.hpp"

#include "roq/htx_futures/gateway/account.hpp"
#include "roq/htx_futures/gateway/request.hpp"
#include "roq/htx_futures/gateway/shared.hpp"

#include "roq/htx_futures/protocol/json/cancel_all_orders_ack.hpp"
#include "roq/htx_futures/protocol/json/cancel_order_ack.hpp"
#include "roq/htx_futures/protocol/json/open_orders_ack.hpp"
#include "roq/htx_futures/protocol/json/place_order_ack.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct OrderEntryREST1 final : public Base<OrderEntryREST1>, public server::OrderActionStream, public web::rest::Client::Handler {
  struct Handler {};

  OrderEntryREST1(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &, Request &);

 protected:
  friend base_type;

  // server::Stream

  void operator()(Trace<Start> const &) override;
  void operator()(Trace<Stop> const &) override;
  void operator()(Trace<Timer> const &) override;

  void operator()(metrics::Writer &) const override;

  void operator()(Trace<ConnectionStatus> const &, std::string_view const &reason = {}) override;

  // server::OrderActionStream

  void operator()(Trace<server::oms::OrderActionBatchBegin> const &) override;
  void operator()(Trace<server::oms::OrderActionBatchCommit> const &) override;
  void operator()(Trace<server::oms::OrderActionBatchRollback> const &) override;

  void operator()(Trace<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id) override;
  void operator()(
      Trace<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  void operator()(
      Trace<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  void operator()(Trace<CancelAllOrders> const &, std::string_view const &request_id) override;

  // web::rest::client::Handler

  void operator()(Trace<web::rest::Connected> const &) override;
  void operator()(Trace<web::rest::Disconnected> const &) override;
  void operator()(Trace<web::rest::Latency> const &) override;

  // open-orders

  void open_orders(TraceInfo const &);
  void open_orders_ack(Trace<web::rest::Response> const &);
  void operator()(Trace<protocol::json::OpenOrdersAck> const &);

  // create-order

  void create_order(Trace<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id);
  void create_order_ack(Trace<web::rest::Response> const &, uint8_t user_id, uint64_t order_id, uint32_t version);
  void operator()(Trace<protocol::json::PlaceOrderAck> const &, uint8_t user_id, uint64_t order_id, uint32_t version);

  // cancel-order

  void cancel_order(
      Trace<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
  void cancel_order_ack(Trace<web::rest::Response> const &, uint8_t user_id, uint64_t order_id, uint32_t version);
  void operator()(Trace<protocol::json::CancelOrderAck> const &, uint8_t user_id, uint64_t order_id, uint32_t version);

  // cancel-all-orders

  void cancel_all_orders(Trace<CancelAllOrders> const &, std::string_view const &request_id);
  void cancel_all_orders_ack(Trace<web::rest::Response> const &, std::string_view const &request_id);
  void operator()(Trace<protocol::json::CancelAllOrdersAck> const &);

  // helpers

  void check_request_queue(TraceInfo const &);

  void process_response(Trace<web::rest::Response> const &, auto error_handler, auto success_handler);

 private:
  [[maybe_unused]] Handler &handler_;
  //
  std::string const name_;
  std::unique_ptr<web::rest::Client> const connection_;
  //
  core::json::BufferStack decode_buffer_;
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile  //
        open_orders,
        open_orders_ack,                 //
        create_order, create_order_ack,  //
        cancel_order, cancel_order_ack,  //
        cancel_all_orders, cancel_all_orders_ack;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
  Account &account_;
  Shared &shared_;
  Request &request_;
  std::string encode_buffer_;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
