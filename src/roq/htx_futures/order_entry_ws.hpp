/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/core/zlib/inflate.hpp"

#include "roq/core/download.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/htx_futures/account.hpp"
#include "roq/htx_futures/order_entry.hpp"
#include "roq/htx_futures/shared.hpp"

#include "roq/htx_futures/json/parser_3.hpp"

namespace roq {
namespace htx_futures {

struct OrderEntryWS final : public OrderEntry, public web::socket::Client::Handler, public json::Parser3::Handler {
  OrderEntryWS(OrderEntry::Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  OrderEntryWS(OrderEntryWS const &) = delete;

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
  // web::socket::Client::Handler

  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

 private:
  void operator()(ConnectionStatus);

  void send_pong(std::chrono::milliseconds timestamp);

  void send_login();

  void parse(std::string_view const &message);

  // json::Parser3::Handler

  void operator()(Trace<json::Close2> const &) override;
  void operator()(Trace<json::Error2> const &) override;
  void operator()(Trace<json::Ping> const &) override;
  void operator()(Trace<json::Auth> const &) override;
  void operator()(Trace<json::Response> const &) override;

 private:
  OrderEntry::Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse,  //
        close, error, ping, auth,   //
        create_order, cancel_order, cancel_all_orders;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
  // account
  Account &account_;
  std::string const auth_path_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};
  // buffers
  std::string encode_buffer_;
  // zlib
  core::zlib::Inflate inflate_;
  std::vector<std::byte> inflate_buffer_;
};

}  // namespace htx_futures
}  // namespace roq
