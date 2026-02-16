/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "roq/server.hpp"

#include "roq/utils/container.hpp"

#include "roq/io/context.hpp"

#include "roq/htx_futures/account.hpp"
#include "roq/htx_futures/config.hpp"
#include "roq/htx_futures/drop_copy.hpp"
#include "roq/htx_futures/market_data.hpp"
#include "roq/htx_futures/order_entry.hpp"
#include "roq/htx_futures/rest.hpp"
#include "roq/htx_futures/settings.hpp"
#include "roq/htx_futures/shared.hpp"
#include "roq/htx_futures/web_socket.hpp"
#include "roq/htx_futures/web_socket_2.hpp"

namespace roq {
namespace htx_futures {

struct Gateway final : public server::Handler,
                       public Rest::Handler,
                       public OrderEntry::Handler,
                       public DropCopy::Handler,
                       public MarketData::Handler,
                       public WebSocket::Handler,
                       public WebSocket2::Handler {
  Gateway(server::Dispatcher &, Settings const &, Config const &, io::Context &);

  Gateway(Gateway const &) = delete;

 protected:
  void operator()(Event<Start> const &) override;
  void operator()(Event<Stop> const &) override;
  void operator()(Event<Timer> const &) override;
  void operator()(Event<Control> const &) override;
  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;

  void operator()(Event<Subscribe> const &) override;

  uint16_t operator()(Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Event<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Event<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id) override;

  uint16_t operator()(Event<MassQuote> const &) override;

  uint16_t operator()(Event<CancelQuotes> const &) override;

  void operator()(metrics::Writer &) const override;

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
  void operator()(Trace<PositionUpdate> const &, bool is_last) override;

  void operator()(Rest::SymbolsUpdate &) override;

  void ensure_symbol_slices(size_t size);

  // utilities

  template <typename... Args>
  void dispatch(Args &&...);

  template <typename... Args>
  static void dispatch_helper(auto &self, Args &&...);

  OrderEntry &get_order_entry(std::string_view const &account);

 private:
  server::Dispatcher &dispatcher_;
  // io
  io::Context &context_;
  // shared
  Shared shared_;
  // accounts
  utils::unordered_map<std::string, std::unique_ptr<Account>> const accounts_;
  // seed
  uint16_t stream_id_ = {};
  // streams
  Rest rest_;
  utils::unordered_map<std::string, std::unique_ptr<OrderEntry>> order_entry_rest_;
  utils::unordered_map<std::string, std::unique_ptr<OrderEntry>> order_entry_ws_;
  utils::unordered_map<std::string, std::unique_ptr<DropCopy>> drop_copy_;
  std::vector<std::unique_ptr<MarketData>> market_data_;
  std::vector<std::unique_ptr<WebSocket>> web_socket_;
  std::vector<std::unique_ptr<WebSocket2>> web_socket_2_;
  // cache
  std::vector<MBPUpdate> bids_, asks_;
};

}  // namespace htx_futures
}  // namespace roq
