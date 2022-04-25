/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "roq/server.hpp"

#include "roq/core/io/context.hpp"

#include "roq/huobi_futures/config.hpp"
#include "roq/huobi_futures/drop_copy.hpp"
#include "roq/huobi_futures/market_data.hpp"
#include "roq/huobi_futures/order_entry.hpp"
#include "roq/huobi_futures/rest.hpp"
#include "roq/huobi_futures/security.hpp"
#include "roq/huobi_futures/shared.hpp"
#include "roq/huobi_futures/web_socket.hpp"
#include "roq/huobi_futures/web_socket_2.hpp"

namespace roq {
namespace huobi_futures {

class Gateway final : public server::Handler,
                      public Rest::Handler,
                      public OrderEntry::Handler,
                      public DropCopy::Handler,
                      public MarketData::Handler,
                      public WebSocket::Handler,
                      public WebSocket2::Handler {
 public:
  Gateway(server::Dispatcher &, const Config &);

 protected:
  void operator()(const Event<Start> &) override;
  void operator()(const Event<Stop> &) override;
  void operator()(const Event<Timer> &) override;
  void operator()(const Event<Connected> &) override;
  void operator()(const Event<Disconnected> &) override;

  uint16_t operator()(
      const Event<CreateOrder> &, const oms::Order &, const std::string_view &request_id) override;
  uint16_t operator()(
      const Event<ModifyOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id) override;
  uint16_t operator()(
      const Event<CancelOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id) override;

  uint16_t operator()(const Event<CancelAllOrders> &, const std::string_view &request_id) override;

  void operator()(metrics::Writer &) override;

  // many

  void operator()(const Trace<StreamStatus const> &) override;
  void operator()(const Trace<ExternalLatency const> &) override;
  void operator()(const Trace<ReferenceData const> &, bool is_last) override;
  void operator()(const Trace<MarketStatus const> &, bool is_last) override;
  void operator()(const Trace<TopOfBook const> &, bool is_last) override;
  void operator()(const Trace<MarketByPriceUpdate const> &, bool is_last, bool refresh) override;
  void operator()(const Trace<TradeSummary const> &, bool is_last) override;
  void operator()(const Trace<StatisticsUpdate const> &, bool is_last) override;
  void operator()(const Trace<FundsUpdate const> &, bool is_last) override;

  void operator()(Rest::SymbolsUpdate &) override;

  void ensure_symbol_slices(size_t size);

  // utilities

  OrderEntry &get_order_entry(const std::string_view &account);

 private:
  server::Dispatcher &dispatcher_;
  // config
  const std::string master_account_;
  // security
  absl::flat_hash_map<Account, std::unique_ptr<Security>> security_;
  // io
  core::io::Context context_;
  // shared
  Shared shared_;
  // seed
  uint16_t stream_id_ = {};
  // streams
  Rest rest_;
  absl::flat_hash_map<Account, std::unique_ptr<OrderEntry>> order_entry_;
  absl::flat_hash_map<Account, std::unique_ptr<DropCopy>> drop_copy_;
  std::vector<std::unique_ptr<MarketData>> market_data_;
  std::vector<std::unique_ptr<WebSocket>> web_socket_;
  std::vector<std::unique_ptr<WebSocket2>> web_socket_2_;
};

}  // namespace huobi_futures
}  // namespace roq
