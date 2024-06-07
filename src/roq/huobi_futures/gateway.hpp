/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "roq/server.hpp"

#include "roq/utils/container.hpp"

#include "roq/io/context.hpp"

#include "roq/huobi_futures/account.hpp"
#include "roq/huobi_futures/config.hpp"
#include "roq/huobi_futures/drop_copy.hpp"
#include "roq/huobi_futures/market_data.hpp"
#include "roq/huobi_futures/order_entry.hpp"
#include "roq/huobi_futures/rest.hpp"
#include "roq/huobi_futures/settings.hpp"
#include "roq/huobi_futures/shared.hpp"
#include "roq/huobi_futures/web_socket.hpp"
#include "roq/huobi_futures/web_socket_2.hpp"

namespace roq {
namespace huobi_futures {

struct Gateway final : public server::Handler,
                       public Rest::Handler,
                       public OrderEntry::Handler,
                       public DropCopy::Handler,
                       public MarketData::Handler,
                       public WebSocket::Handler,
                       public WebSocket2::Handler {
  Gateway(server::Dispatcher &, Settings const &, Config const &, io::Context &);

 protected:
  void operator()(Event<Start> const &) override;
  void operator()(Event<Stop> const &) override;
  void operator()(Event<Timer> const &) override;
  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;

  uint16_t operator()(Event<CreateOrder> const &, server::oms::Order const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Event<ModifyOrder> const &, server::oms::Order const &, std::string_view const &request_id, std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Event<CancelOrder> const &, server::oms::Order const &, std::string_view const &request_id, std::string_view const &previous_request_id) override;

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id) override;

  void operator()(metrics::Writer &) override;

  // many

  void operator()(Trace<StreamStatus> const &) override;
  void operator()(Trace<ExternalLatency> const &) override;
  void operator()(Trace<ReferenceData> const &, bool is_last) override;
  void operator()(Trace<MarketStatus> const &, bool is_last) override;
  void operator()(Trace<TopOfBook> const &, bool is_last) override;
  void operator()(Trace<MarketByPriceUpdate> const &, bool is_last) override;
  void operator()(Trace<TradeSummary> const &, bool is_last) override;
  void operator()(Trace<StatisticsUpdate> const &, bool is_last) override;
  void operator()(Trace<FundsUpdate> const &, bool is_last) override;

  void operator()(Rest::SymbolsUpdate &) override;

  void ensure_symbol_slices(size_t size);

  // utilities

  template <typename... Args>
  void dispatch(Args &&...);

  OrderEntry &get_order_entry(std::string_view const &account);

 private:
  server::Dispatcher &dispatcher_;
  // accounts
  utils::unordered_map<std::string, std::unique_ptr<Account>> const accounts_;
  // io
  io::Context &context_;
  // shared
  Shared shared_;
  // seed
  uint16_t stream_id_ = {};
  // streams
  Rest rest_;
  utils::unordered_map<std::string, std::unique_ptr<OrderEntry>> order_entry_;
  utils::unordered_map<std::string, std::unique_ptr<DropCopy>> drop_copy_;
  std::vector<std::unique_ptr<MarketData>> market_data_;
  std::vector<std::unique_ptr<WebSocket>> web_socket_;
  std::vector<std::unique_ptr<WebSocket2>> web_socket_2_;
  // cache
  std::vector<MBPUpdate> bids_, asks_;
};

}  // namespace huobi_futures
}  // namespace roq
