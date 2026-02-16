/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/core/download.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/htx_futures/account.hpp"
#include "roq/htx_futures/order_entry.hpp"
#include "roq/htx_futures/order_entry_state.hpp"
#include "roq/htx_futures/shared.hpp"

#include "roq/htx_futures/json/cancel_all_orders_ack.hpp"
#include "roq/htx_futures/json/cancel_order_ack.hpp"
#include "roq/htx_futures/json/open_orders_ack.hpp"
#include "roq/htx_futures/json/place_order_ack.hpp"

namespace roq {
namespace htx_futures {

struct OrderEntryREST final : public OrderEntry, public web::rest::Client::Handler {
  OrderEntryREST(OrderEntry::Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  OrderEntryREST(OrderEntry const &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &) override;
  void operator()(Event<Stop> const &) override;
  void operator()(Event<Timer> const &) override;

  void operator()(metrics::Writer &) const override;

  uint16_t operator()(Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Event<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Event<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id) override;

 protected:
  // web::rest::client::Handler

  void operator()(Trace<web::rest::Client::Connected> const &) override;
  void operator()(Trace<web::rest::Client::Disconnected> const &) override;
  void operator()(Trace<web::rest::Client::Latency> const &) override;

  void operator()(ConnectionStatus);

  uint32_t download(OrderEntryState);

  // open-orders

  void open_orders();
  void open_orders_ack(Trace<web::rest::Response> const &);
  void operator()(Trace<json::OpenOrdersAck> const &);

  // create-order

  void create_order(Event<CreateOrder> const &, server::oms::Order const &, std::string_view const &request_id);
  void create_order_ack(Trace<web::rest::Response> const &, uint8_t user_id, uint64_t order_id, uint32_t version);
  void operator()(Trace<json::PlaceOrderAck> const &, uint8_t user_id, uint64_t order_id, uint32_t version);

  // cancel-order

  void cancel_order(Event<CancelOrder> const &, server::oms::Order const &, std::string_view const &request_id, std::string_view const &previous_request_id);
  void cancel_order_ack(Trace<web::rest::Response> const &, uint8_t user_id, uint64_t order_id, uint32_t version);
  void operator()(Trace<json::CancelOrderAck> const &, uint8_t user_id, uint64_t order_id, uint32_t version);

  // cancel-all-orders

  void cancel_all_orders(Event<CancelAllOrders> const &, std::string_view const &request_id);
  void cancel_all_orders_ack(Trace<web::rest::Response> const &, std::string_view const &request_id);
  void operator()(Trace<json::CancelAllOrdersAck> const &);

  // helpers

  void process_response(web::rest::Response const &, auto error_handler, auto success_handler);

  template <typename... Args>
  void operator()(Trace<server::oms::Response> const &, uint8_t user_id, uint64_t order_id, Args &&...);

  void operator()(Trace<server::oms::OrderUpdate> const &, std::string_view const &client_order_id);

 private:
  OrderEntry::Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // connection
  std::unique_ptr<web::rest::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
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
  // account
  Account &account_;
  // shared
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};
  core::Download<OrderEntryState> download_;
  // buffers
  std::string encode_buffer_;
};

}  // namespace htx_futures
}  // namespace roq
