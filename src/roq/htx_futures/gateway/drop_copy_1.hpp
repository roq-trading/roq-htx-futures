/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <vector>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/core/zlib/inflate.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/server/stream.hpp"

#include "roq/htx_futures/gateway/account.hpp"
#include "roq/htx_futures/gateway/shared.hpp"

#include "roq/htx_futures/protocol/json/parser_2.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct DropCopy1 final : Base<DropCopy1>, public server::Stream, public web::socket::Client::Handler, public protocol::json::Parser2::Handler {
  struct Handler {};

  DropCopy1(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  // protected:
  friend base_type;

  // server::Stream

  uint16_t stream_id() const override { return stream_id_; }

  bool ready() const override { return connection_status_ == ConnectionStatus::READY; }

  void operator()(Trace<Start> const &) override;
  void operator()(Trace<Stop> const &) override;
  void operator()(Trace<Timer> const &) override;

  void operator()(metrics::Writer &) const override;

  void operator()(Trace<ConnectionStatus> const &, std::string_view const &reason = {}) override;

 protected:
  // web::socket::Client::Handler

  void operator()(Trace<web::socket::Connected> const &) override;
  void operator()(Trace<web::socket::Disconnected> const &) override;
  void operator()(Trace<web::socket::Latency> const &) override;
  void operator()(Trace<web::socket::Ready> const &) override;
  void operator()(Trace<web::socket::Close> const &) override;
  void operator()(Trace<web::socket::Text> const &) override;
  void operator()(Trace<web::socket::Binary> const &) override;

  // protocol::json::Parser2::Handler

  void operator()(Trace<protocol::json::Close2> const &) override;
  void operator()(Trace<protocol::json::Error2> const &) override;
  void operator()(Trace<protocol::json::Ping> const &) override;
  void operator()(Trace<protocol::json::Auth> const &) override;
  void operator()(Trace<protocol::json::Sub> const &) override;
  void operator()(Trace<protocol::json::FundingRate> const &) override;
  void operator()(Trace<protocol::json::Accounts> const &) override;
  void operator()(Trace<protocol::json::Positions> const &) override;
  void operator()(Trace<protocol::json::MatchOrders> const &) override;
  void operator()(Trace<protocol::json::Orders> const &) override;
  void operator()(Trace<protocol::json::AccountsCross> const &) override;
  void operator()(Trace<protocol::json::PositionsCross> const &) override;
  void operator()(Trace<protocol::json::MatchOrdersCross> const &) override;
  void operator()(Trace<protocol::json::OrdersCross> const &) override;

  // helpers

  void send_pong(std::chrono::milliseconds timestamp);

  void send_login();

  void subscribe();
  void subscribe(std::string_view const &topic);

  void parse(std::string_view const &message);

 private:
  [[maybe_unused]] Handler &handler_;
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
    utils::metrics::Profile parse, close, error, ping, auth, sub, accounts, positions, match_orders, orders;
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
  ConnectionStatus connection_status_ = {};
  // zlib
  core::zlib::Inflate inflate_;
  std::vector<std::byte> inflate_buffer_;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
