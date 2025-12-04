/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/core/zlib/inflate.hpp"

#include "roq/core/download.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/htx_futures/account.hpp"
#include "roq/htx_futures/shared.hpp"

#include "roq/htx_futures/json/parser_2.hpp"

namespace roq {
namespace htx_futures {

struct DropCopy final : public web::socket::Client::Handler, public json::Parser2::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<FundsUpdate> const &, bool is_last) = 0;
    virtual void operator()(Trace<PositionUpdate> const &, bool is_last) = 0;
  };

  DropCopy(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &);

  DropCopy(DropCopy const &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

 protected:
  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

 private:
  void operator()(ConnectionStatus);

  void send_pong(std::chrono::milliseconds timestamp);

  void send_login();

  void subscribe();
  void subscribe(std::string_view const &topic);

  void parse(std::string_view const &message);

  void operator()(Trace<json::Close2> const &) override;
  void operator()(Trace<json::Error2> const &) override;
  void operator()(Trace<json::Ping> const &) override;
  void operator()(Trace<json::Auth> const &) override;
  void operator()(Trace<json::Sub> const &) override;
  void operator()(Trace<json::FundingRate> const &) override;
  void operator()(Trace<json::Accounts> const &) override;
  void operator()(Trace<json::Positions> const &) override;
  void operator()(Trace<json::MatchOrders> const &) override;
  void operator()(Trace<json::Orders> const &) override;
  void operator()(Trace<json::AccountsCross> const &) override;
  void operator()(Trace<json::PositionsCross> const &) override;
  void operator()(Trace<json::MatchOrdersCross> const &) override;
  void operator()(Trace<json::OrdersCross> const &) override;

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse, close, error, ping, auth, sub, accounts, positions, match_orders, orders;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
  // account
  Account &account_;
  std::string const auth_path_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};
  // zlib
  core::zlib::Inflate inflate_;
  std::vector<std::byte> inflate_buffer_;
};

}  // namespace htx_futures
}  // namespace roq
