/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/web/client_socket.h"

#include "roq/core/zlib/inflate.h"

#include "roq/download.h"
#include "roq/server.h"

#include "roq/huobi_futures/shared.h"

#include "roq/huobi_futures/json/parser.h"

namespace roq {
namespace huobi_futures {

class MarketData final : public core::web::ClientSocket::Handler, public json::Parser::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<TopOfBook> &, bool is_last) = 0;
    virtual void operator()(
        const server::Trace<MarketByPriceUpdate> &, bool is_last, bool refresh) = 0;
    virtual void operator()(const server::Trace<TradeSummary> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<StatisticsUpdate> &, bool is_last) = 0;
  };

  MarketData(Handler &, core::io::Context &, uint32_t stream_id, Shared &, size_t index);

  MarketData(MarketData &&) = delete;
  MarketData(const MarketData &) = delete;

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

  void subscribe(const std::span<std::string const> &symbols);
  void subscribe(
      const std::span<std::string const> &symbols,
      const std::string_view &source,
      const std::string_view &theme);
  void subscribe(
      const std::span<std::string const> &symbols,
      const std::string_view &source,
      const std::string_view &theme,
      const std::string_view &data_type);

  void send_pong(std::chrono::milliseconds timestamp);

  void parse(const std::string_view &message);

  void operator()(const server::Trace<json::Ping> &) override;

  void operator()(const server::Trace<json::Error> &) override;
  void operator()(const server::Trace<json::Subbed> &) override;

  void operator()(const server::Trace<json::BBO> &) override;
  void operator()(const server::Trace<json::Depth> &) override;
  void operator()(const server::Trace<json::Trade> &) override;
  void operator()(const server::Trace<json::Detail> &) override;

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
    core::metrics::Profile parse, ping, error, subbed, bbo, depth, trade, detail;
  } profile_;
  struct {
    core::metrics::Latency ping, heartbeat;
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
