/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/web/socket.h"

#include "roq/download.h"
#include "roq/server.h"

#include "roq/huobi/market_data_state.h"
#include "roq/huobi/shared.h"

#include "roq/huobi/json/market_stream_parser.h"

namespace roq {
namespace huobi {

class MarketData final : public core::web::Socket::Handler,
                         public json::MarketStreamParser::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<TopOfBook> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<MarketByPriceUpdate> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<TradeSummary> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<StatisticsUpdate> &, bool is_last) = 0;
  };

  MarketData(Handler &, core::io::Context &, uint32_t stream_id, Shared &);

  MarketData(MarketData &&) = delete;
  MarketData(const MarketData &) = delete;

  bool ready() const;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  void update_subscriptions(std::vector<std::string> &symbols);

 protected:
  void operator()(const core::web::Socket::Connected &) override;
  void operator()(const core::web::Socket::Disconnected &) override;
  void operator()(const core::web::Socket::Ready &) override;
  void operator()(const core::web::Socket::Close &) override;
  void operator()(const core::web::Socket::Latency &) override;
  void operator()(const core::web::Socket::Text &) override;

 private:
  void operator()(ConnectionStatus);

  uint32_t download(MarketDataState);

  void subscribe(const roq::span<std::string> &symbols);

  void subscribe_agg_trade(const roq::span<std::string> &symbols);
  void subscribe_trade(const roq::span<std::string> &symbols);
  void subscribe_mini_ticker(const roq::span<std::string> &symbols);
  void subscribe_book_ticker(const roq::span<std::string> &symbols);
  void subscribe_depth(const roq::span<std::string> &symbols);

  void parse(const std::string_view &message);

  // response
  void operator()(int32_t, const json::Error &) override;
  void operator()(int32_t, const json::Result &) override;

  // update
  void operator()(const json::AggTrade &, const server::TraceInfo &) override;
  void operator()(const json::Trade &, const server::TraceInfo &) override;
  void operator()(const json::MiniTicker &, const server::TraceInfo &) override;
  void operator()(const json::BookTicker &, const server::TraceInfo &) override;
  void operator()(
      const std::string_view &symbol, const json::Depth &depth, const server::TraceInfo &) override;
  void operator()(
      const std::string_view &symbol,
      const json::DepthUpdate &,
      const server::TraceInfo &) override;

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  // web socket
  core::web::Socket connection_;
  // buffers
  core::Buffer decode_buffer_;
  // session
  uint64_t request_id_ = {};
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse, error, result, agg_trade, trade, mini_ticker, book_ticker, depth,
        depth_update;
  } profile_;
  struct {
    core::metrics::Latency ping, heartbeat;
  } latency_;
  // cache
  Shared &shared_;
  std::vector<std::string> symbols_;
  // state
  bool ready_ = false;
  ConnectionStatus status_ = {};
  server::Download<MarketDataState> download_;
};

}  // namespace huobi
}  // namespace roq
