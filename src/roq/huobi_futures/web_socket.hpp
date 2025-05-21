/* Copyright (c) 2017-2025, Hans Erik Thrane */

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

#include "roq/server.hpp"

#include "roq/huobi_futures/shared.hpp"

#include "roq/huobi_futures/json/parser.hpp"

namespace roq {
namespace huobi_futures {

struct WebSocket final : public web::socket::Client::Handler, public json::Parser::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<StatisticsUpdate> const &, bool is_last) = 0;
  };

  WebSocket(Handler &, io::Context &, uint16_t stream_id, Shared &, size_t index);

  WebSocket(WebSocket const &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  void subscribe(size_t start_from = 0);

 protected:
  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

 private:
  void operator()(ConnectionStatus);

  void subscribe(std::span<Symbol const> const &symbols);
  void subscribe(std::span<Symbol const> const &symbols, std::string_view const &source, std::string_view const &theme);

  void send_pong(std::chrono::milliseconds timestamp);

  void parse(std::string_view const &message);

  void operator()(Trace<json::Ping> const &) override;

  void operator()(Trace<json::Error> const &) override;
  void operator()(Trace<json::Subbed> const &) override;

  void operator()(Trace<json::BBO> const &) override;
  void operator()(Trace<json::Depth> const &) override;
  void operator()(Trace<json::Trade> const &) override;
  void operator()(Trace<json::Detail> const &) override;

  void operator()(Trace<json::EstimatedRate> const &) override;
  void operator()(Trace<json::PremiumIndex> const &) override;
  void operator()(Trace<json::Basis> const &) override;
  void operator()(Trace<json::Index> const &) override;

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  size_t const index_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  std::vector<std::byte> decode_buffer_;
  // session
  uint64_t request_id_ = {};
  // metrics
  struct {
    utils::metrics::Counter disconnect, total_bytes_received;
  } counter_;
  struct {
    utils::metrics::Profile parse, ping, error, subbed, estimated_rate, premium_index, basis, index;
  } profile_;
  struct {
    utils::metrics::Latency ping, heartbeat;
  } latency_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};
  // zlib
  core::zlib::Inflate inflate_;
  std::vector<std::byte> inflate_buffer_;
};

}  // namespace huobi_futures
}  // namespace roq
