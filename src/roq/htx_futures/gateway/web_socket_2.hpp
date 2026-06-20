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

#include "roq/htx_futures/gateway/shared.hpp"

#include "roq/htx_futures/protocol/json/parser_2.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct WebSocket2 final : public web::socket::Client::Handler, public protocol::json::Parser2::Handler {
  struct Handler {};

  WebSocket2(Handler &, io::Context &, uint16_t stream_id, Shared &, size_t index);

  WebSocket2(WebSocket2 const &) = delete;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  void subscribe(size_t start_from = 0);

 protected:
  // web::socket::Client::Handler

  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

  // helpers

  bool ready() const { return connection_status_ == ConnectionStatus::READY; }

  void operator()(ConnectionStatus, std::string_view const &reason = {});

  void subscribe(std::span<Symbol const> const &symbols);
  void subscribe(std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme);

  void send_pong(std::chrono::milliseconds timestamp);

  void parse(std::string_view const &message);

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

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  size_t const index_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect, total_bytes_received;
  } counter_;
  struct {
    utils::metrics::Profile parse, close, error, ping, sub, funding_rate;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
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
