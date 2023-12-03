/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/download.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/server.hpp"

#include "roq/huobi_futures/rest_state.hpp"
#include "roq/huobi_futures/shared.hpp"

#include "roq/huobi_futures/json/contract_info.hpp"

namespace roq {
namespace huobi_futures {

struct Rest final : public web::rest::Client::Handler {
  struct SymbolsUpdate final {
    std::vector<Symbol> &symbols;
  };

  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<ReferenceData> const &, bool is_last) = 0;
    virtual void operator()(Trace<MarketStatus> const &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  Rest(Handler &, io::Context &context, uint16_t stream_id, Shared &);

  Rest(Rest &&) = delete;
  Rest(Rest const &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &);

 protected:
  void operator()(Trace<web::rest::Client::Connected> const &) override;
  void operator()(Trace<web::rest::Client::Disconnected> const &) override;
  void operator()(Trace<web::rest::Client::Latency> const &) override;
  void operator()(Trace<web::rest::Response> const &, uint64_t request_id, uint64_t opaque) override;

  void operator()(ConnectionStatus);

  uint32_t download(RestState);

  void get_contract_info();
  void get_contract_info_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<json::ContractInfo> const &);

  template <typename SuccessHandler, typename ErrorHandler>
  void process_response(web::rest::Response const &, SuccessHandler, ErrorHandler);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // connection
  std::unique_ptr<web::rest::Client> const connection_;
  // buffers
  std::vector<std::byte> decode_buffer_;
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
  // experimental
  std::chrono::nanoseconds next_refresh_ = {};
};

}  // namespace huobi_futures
}  // namespace roq
