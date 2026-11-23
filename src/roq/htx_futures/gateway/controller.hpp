/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>
#include <string>
#include <vector>

#include "roq/server.hpp"

#include "roq/utils/container.hpp"

#include "roq/io/context.hpp"

#include "roq/htx_futures/gateway/account.hpp"
#include "roq/htx_futures/gateway/config.hpp"
#include "roq/htx_futures/gateway/request.hpp"
#include "roq/htx_futures/gateway/settings.hpp"
#include "roq/htx_futures/gateway/shared.hpp"

#include "roq/htx_futures/gateway/drop_copy_1.hpp"
#include "roq/htx_futures/gateway/drop_copy_5.hpp"
#include "roq/htx_futures/gateway/market_data.hpp"
#include "roq/htx_futures/gateway/order_entry_rest_1.hpp"
#include "roq/htx_futures/gateway/order_entry_rest_5.hpp"
#include "roq/htx_futures/gateway/order_entry_ws_1.hpp"
#include "roq/htx_futures/gateway/order_entry_ws_5.hpp"
#include "roq/htx_futures/gateway/rest.hpp"
#include "roq/htx_futures/gateway/web_socket.hpp"
#include "roq/htx_futures/gateway/web_socket_2.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct Controller final : public Base<Controller>,
                          public server::Handler,
                          public Rest::Handler,
                          public OrderEntryREST1::Handler,
                          public OrderEntryREST5::Handler,
                          public OrderEntryWS1::Handler,
                          public OrderEntryWS5::Handler,
                          public DropCopy1::Handler,
                          public DropCopy5::Handler,
                          public MarketData::Handler,
                          public WebSocket::Handler,
                          public WebSocket2::Handler {
  ROQ_PUBLIC static std::unique_ptr<server::Handler> create(server::Dispatcher &, Settings const &, Config const &, io::Context &);

  ROQ_PUBLIC static uint8_t parse_api(Settings const &);

  Controller(server::Dispatcher &, Settings const &, Config const &, io::Context &);

  Controller(Controller const &) = delete;

 protected:
  friend base_type;

  // server::Handler

  void operator()(Trace<Start> const &) override;
  void operator()(Trace<Stop> const &) override;
  void operator()(Trace<Timer> const &) override;

  void operator()(Trace<Connected> const &, uint8_t user_id) override;
  void operator()(Trace<Disconnected> const &, uint8_t user_id) override;

  void operator()(Trace<Subscribe> const &) override;
  void operator()(Trace<Control> const &) override;

  void operator()(Trace<server::oms::OrderActionBatchBegin> const &) override;
  void operator()(Trace<server::oms::OrderActionBatchCommit> const &) override;
  void operator()(Trace<server::oms::OrderActionBatchRollback> const &) override;

  uint16_t operator()(Trace<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Trace<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Trace<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Trace<CancelAllOrders> const &, std::string_view const &request_id) override;

  uint16_t operator()(Trace<MassQuote> const &) override;

  uint16_t operator()(Trace<CancelQuotes> const &) override;

  void operator()(metrics::Writer &) const override;

  // Rest::Handler

  void operator()(Trace<SymbolsUpdate> const &) override;

  // helpers

  void ensure_symbol_slices(TraceInfo const &, size_t size);

  template <typename... Args>
  void dispatch(Args &&...);

  template <typename... Args>
  static void dispatch_helper(auto &self, Args &&...);

  template <typename T, typename... Args>
  uint16_t dispatch_order_action(Trace<T> const &, Args &&...);

 private:
  server::Dispatcher &dispatcher_;
  //
  io::Context &context_;
  Shared shared_;
  uint16_t stream_id_ = {};
  //
  utils::unordered_map<std::string, std::unique_ptr<Account>> const accounts_;
  utils::unordered_map<std::string, std::unique_ptr<Request>> const requests_;
  //
  std::unique_ptr<server::Stream> rest_;
  utils::unordered_map<std::string, std::unique_ptr<server::OrderActionStream>> order_entry_rest_;
  utils::unordered_map<std::string, std::unique_ptr<server::OrderActionStream>> order_entry_ws_;
  utils::unordered_map<std::string, std::unique_ptr<server::Stream>> drop_copy_;
  std::vector<std::unique_ptr<server::MarketDataStream>> market_data_;
  std::vector<std::unique_ptr<server::MarketDataStream>> web_socket_;
  std::vector<std::unique_ptr<server::MarketDataStream>> web_socket_2_;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
