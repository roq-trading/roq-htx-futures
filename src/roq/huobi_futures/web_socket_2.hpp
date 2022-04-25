/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/download.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/web/client_socket.hpp"

#include "roq/core/zlib/inflate.hpp"

#include "roq/server.hpp"

#include "roq/huobi_futures/shared.hpp"

#include "roq/huobi_futures/json/parser_2.hpp"

namespace roq {
namespace huobi_futures {

class WebSocket2 final : public core::web::ClientSocket::Handler, public json::Parser2::Handler {
 public:
  struct Handler {
    virtual void operator()(const Trace<StreamStatus const> &) = 0;
    virtual void operator()(const Trace<ExternalLatency const> &) = 0;
    virtual void operator()(const Trace<StatisticsUpdate const> &, bool is_last) = 0;
  };

  WebSocket2(Handler &, core::io::Context &, uint16_t stream_id, Shared &, size_t index);

  WebSocket2(WebSocket2 &&) = delete;
  WebSocket2(const WebSocket2 &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  void subscribe(size_t start_from = 0);

 protected:
  void operator()(const core::web::ClientSocket::Connected &) override;
  void operator()(const core::web::ClientSocket::Disconnected &) override;
  void operator()(const core::web::ClientSocket::Ready &) override;
  void operator()(const core::web::ClientSocket::Close &) override;
  void operator()(const core::web::ClientSocket::Latency &) override;
  void operator()(const core::web::ClientSocket::Text &) override;
  void operator()(const core::web::ClientSocket::Binary &) override;

 private:
  void operator()(ConnectionStatus);

  void subscribe(const std::span<Symbol const> &symbols);
  void subscribe(
      const std::span<Symbol const> &symbols,
      const std::string_view &source,
      const std::string_view &theme);

  void send_pong(std::chrono::milliseconds timestamp);

  void parse(const std::string_view &message);

  void operator()(const Trace<json::Ping const> &) override;
  void operator()(const Trace<json::Close const> &) override;
  void operator()(const Trace<json::FundingRate const> &) override;

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const size_t index_;
  // web socket
  core::web::ClientSocket connection_;
  // buffers
  core::Buffer decode_buffer_;
  // session
  uint64_t request_id_ = {};
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, ping, close, funding_rate;
  } profile_;
  struct {
    core::metrics::Latency ping;
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
