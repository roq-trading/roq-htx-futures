/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/buffer.hpp"
#include "roq/core/download.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/web/client.hpp"

#include "roq/server.hpp"

#include "roq/huobi_futures/rest_state.hpp"
#include "roq/huobi_futures/shared.hpp"

#include "roq/huobi_futures/json/contract_info.hpp"

namespace roq {
namespace huobi_futures {

class Rest final : public core::web::Client::Handler {
 public:
  struct SymbolsUpdate final {
    std::vector<Symbol> &symbols;
  };

  struct Handler {
    virtual void operator()(Trace<StreamStatus const> const &) = 0;
    virtual void operator()(Trace<ExternalLatency const> const &) = 0;
    virtual void operator()(Trace<ReferenceData const> const &, bool is_last) = 0;
    virtual void operator()(Trace<MarketStatus const> const &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  Rest(Handler &, core::io::Context &context, uint16_t stream_id, Shared &);

  Rest(Rest &&) = delete;
  Rest(Rest const &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &);

 protected:
  void operator()(core::web::Client::Connected const &) override;
  void operator()(core::web::Client::Disconnected const &) override;
  void operator()(core::web::Client::Latency const &) override;

  void operator()(ConnectionStatus);

  uint32_t download(RestState);

  void get_contract_info();
  void get_contract_info_ack(Trace<core::web::Response const> const &, uint32_t sequence);
  void operator()(Trace<json::ContractInfo const> const &);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  // connection
  core::web::Client connection_;
  // buffers
  core::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile contract_info, contract_info_ack;
  } profile_;
  struct {
    core::metrics::Latency ping;
  } latency_;
  // cache
  Shared &shared_;
  absl::flat_hash_set<Symbol> all_symbols_;
  // state
  ConnectionStatus status_ = {};
  core::Download<RestState> download_;
};

}  // namespace huobi_futures
}  // namespace roq
