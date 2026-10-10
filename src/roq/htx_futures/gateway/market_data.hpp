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

#include "roq/htx_futures/gateway/shared.hpp"

#include "roq/htx_futures/protocol/json/parser.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct MarketData final : public Base<MarketData>,
                          public server::MarketDataStream,
                          public web::socket::Client::Handler,
                          public protocol::json::Parser::Handler {
  struct Handler {};

  MarketData(Handler &, io::Context &, uint16_t stream_id, Shared &, size_t index);

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

  // server::MarketDataStream

  void subscribe(size_t start_from = 0) override;

 protected:
  // web::socket::Client::Handler

  void operator()(Trace<web::socket::Connected> const &) override;
  void operator()(Trace<web::socket::Disconnected> const &) override;
  void operator()(Trace<web::socket::Latency> const &) override;
  void operator()(Trace<web::socket::Ready> const &) override;
  void operator()(Trace<web::socket::Close> const &) override;
  void operator()(Trace<web::socket::Text> const &) override;
  void operator()(Trace<web::socket::Binary> const &) override;

  // protocol::json::Parser::Handler

  void operator()(Trace<protocol::json::Ping> const &) override;

  void operator()(Trace<protocol::json::Error> const &) override;
  void operator()(Trace<protocol::json::Subbed> const &) override;

  void operator()(Trace<protocol::json::BBO> const &) override;
  void operator()(Trace<protocol::json::Depth> const &) override;
  void operator()(Trace<protocol::json::Trade> const &) override;
  void operator()(Trace<protocol::json::Detail> const &) override;

  void operator()(Trace<protocol::json::EstimatedRate> const &) override;
  void operator()(Trace<protocol::json::PremiumIndex> const &) override;
  void operator()(Trace<protocol::json::Basis> const &) override;
  void operator()(Trace<protocol::json::Index> const &) override;

  // helpers

  void subscribe(std::span<Symbol const> const &symbols);
  void subscribe(std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme);
  void subscribe_with_data_type(
      std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme, std::string_view const &data_type);

  void send_pong(std::chrono::milliseconds timestamp);

  void parse(std::string_view const &message);

 private:
  [[maybe_unused]] Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  size_t const index_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // session
  uint64_t request_id_ = {};
  // metrics
  struct {
    utils::metrics::Counter disconnect, total_bytes_received;
  } counter_;
  struct {
    utils::metrics::Profile parse, ping, error, subbed, bbo, depth, trade, detail;
  } profile_;
  struct {
    utils::metrics::Latency ping, heartbeat;
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
