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

#include "roq/huobi_futures/security.hpp"
#include "roq/huobi_futures/shared.hpp"

#include "roq/huobi_futures/json/parser_2.hpp"

namespace roq {
namespace huobi_futures {

class DropCopy final : public core::web::ClientSocket::Handler, public json::Parser2::Handler {
 public:
  struct Handler {
    virtual void operator()(const Trace<StreamStatus> &) = 0;
    virtual void operator()(const Trace<ExternalLatency> &) = 0;
    virtual void operator()(const Trace<FundsUpdate> &, bool is_last) = 0;
  };

  DropCopy(Handler &, core::io::Context &, uint16_t stream_id, Security &, Shared &);

  DropCopy(DropCopy &&) = delete;
  DropCopy(const DropCopy &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

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

  void send_pong(std::chrono::milliseconds timestamp);

  void parse(const std::string_view &message);

  void operator()(const Trace<json::Ping> &) override;
  void operator()(const Trace<json::Close> &) override;
  void operator()(const Trace<json::FundingRate> &) override;

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
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
    core::metrics::Profile parse, ping, close;
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
  // zlib
  core::zlib::Inflate inflate_;
  std::vector<std::byte> inflate_buffer_;
};

}  // namespace huobi_futures
}  // namespace roq
