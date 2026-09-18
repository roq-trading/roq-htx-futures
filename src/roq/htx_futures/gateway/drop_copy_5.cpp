/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/gateway/drop_copy_5.hpp"

#include "roq/mask.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/htx_futures/protocol/json/map.hpp"
#include "roq/htx_futures/protocol/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "dc"sv;

auto const SUPPORTS = Mask{
    SupportType::FUNDS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 2;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.ws.order_uri;
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = {},
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.ping_freq,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::socket::Client::create(handler, context, config, []() { return std::string(); });
}

auto create_auth_path(auto &settings) {
  return settings.ws.order_uri.get_path();
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

DropCopy5::DropCopy5(DropCopy::Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
          .close = create_metrics(shared.settings, name_, "close"sv),
          .error = create_metrics(shared.settings, name_, "error"sv),
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .auth = create_metrics(shared.settings, name_, "auth"sv),
          .sub = create_metrics(shared.settings, name_, "sub"sv),
          .accounts = create_metrics(shared.settings, name_, "accounts"sv),
          .positions = create_metrics(shared.settings, name_, "positions"sv),
          .match_orders = create_metrics(shared.settings, name_, "match_orders"sv),
          .orders = create_metrics(shared.settings, name_, "orders"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      account_{account}, auth_path_{create_auth_path(shared.settings)}, shared_{shared} {
}

void DropCopy5::operator()(Event<Start> const &) {
  (*connection_).start();
}

void DropCopy5::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void DropCopy5::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
}

void DropCopy5::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.close, metrics::Type::PROFILE)
      .write(profile_.error, metrics::Type::PROFILE)
      .write(profile_.ping, metrics::Type::PROFILE)
      .write(profile_.auth, metrics::Type::PROFILE)
      .write(profile_.sub, metrics::Type::PROFILE)
      .write(profile_.accounts, metrics::Type::PROFILE)
      .write(profile_.positions, metrics::Type::PROFILE)
      .write(profile_.match_orders, metrics::Type::PROFILE)
      .write(profile_.orders, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

void DropCopy5::operator()(web::socket::Client::Connected const &) {
}

void DropCopy5::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void DropCopy5::operator()(web::socket::Client::Ready const &) {
  send_login();
  (*this)(ConnectionStatus::LOGIN_SENT);
}

void DropCopy5::operator()(web::socket::Client::Close const &) {
}

void DropCopy5::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.name,
      .latency = latency.sample,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy5::operator()(web::socket::Client::Text const &text) {
  log::info<5>(R"(message="{}")"sv, text.payload);
  parse(text.payload);
}

void DropCopy5::operator()(web::socket::Client::Binary const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy5::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = account_.name,
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::WS,
      .encoding = {Encoding::JSON},
      .priority = Priority::PRIMARY,
      .connection_status = connection_status_,
      .reason = reason,
      .interface = (*connection_).get_interface(),
      .authority = (*connection_).get_current_authority(),
      .path = (*connection_).get_current_path(),
      .proxy = (*connection_).get_proxy(),
  };
  log::info("stream_status={}"sv, stream_status);
  create_trace_and_dispatch(shared_.dispatcher, trace_info, stream_status);
}

void DropCopy5::send_pong(std::chrono::milliseconds timestamp) {
  auto message = fmt::format(
      R"({{)"
      R"("op":"pong",)"
      R"("ts":{})"
      R"(}})"sv,
      timestamp.count());
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void DropCopy5::send_login() {
  auto now_utc = clock::get_realtime<std::chrono::seconds>();
  auto message = account_.create_ws_auth(auth_path_, now_utc);
  // log::warn("DEBUG {}"sv, message);
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void DropCopy5::subscribe() {
  switch (account_.margin_mode) {
    using enum MarginMode;
    case UNDEFINED:
      break;
    case ISOLATED:
      log::fatal("Unexpected: UNTESTED"sv);
      break;
    case CROSS:
      subscribe(shared_.api.order_management.topic_accounts);
      for (auto &symbol : shared_.settings.download.symbols) {
        subscribe(shared_.api.order_management.topic_positions, symbol);
        subscribe(shared_.api.order_management.topic_match_orders, symbol);
        subscribe(shared_.api.order_management.topic_orders, symbol);
      }
      break;
    case PORTFOLIO:
      break;
  }
}

void DropCopy5::subscribe(std::string_view const &topic) {
  auto message = fmt::format(
      R"({{)"
      R"("op":"sub",)"
      R"("topic":"{}")"
      R"(}})"sv,
      topic);
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void DropCopy5::subscribe(std::string_view const &topic, std::string_view const &contract_code) {
  auto message = fmt::format(
      R"({{)"
      R"("op":"sub",)"
      R"("topic":"{}",)"
      R"("contract_code":"{}")"
      R"(}})"sv,
      topic,
      contract_code);
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void DropCopy5::parse(std::string_view const &message) {
  profile_.parse([&]() {
    log::info<5>(R"(message="{}")"sv, message);
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    try {
      TraceInfo trace_info;
      if (!protocol::json::Parser5::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

void DropCopy5::operator()(Trace<protocol::json::Close2> const &) {
  profile_.close([&]() {
    log::warn("Exchange requested connection closed"sv);
    (*connection_).close();
  });
}

void DropCopy5::operator()(Trace<protocol::json::Error2> const &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::error("error={}"sv, error);
    (*connection_).close();
  });
}

void DropCopy5::operator()(Trace<protocol::json::Ping> const &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    send_pong(ping.timestamp);
  });
}

void DropCopy5::operator()(Trace<protocol::json::Auth> const &event) {
  profile_.auth([&]() {
    auto &[trace_info, auth] = event;
    if (auth.err_code == 0) {
      subscribe();
      (*this)(ConnectionStatus::READY);
    } else {
      if (shared_.settings.experimental.retry_logon) {
        log::error("[{}] auth={}"sv, account_.name, auth);
        log::warn("Disconnecting..."sv);
        (*connection_).close();
      } else {
        log::fatal("[{}] auth={}"sv, account_.name, auth);
      }
    }
  });
}

void DropCopy5::operator()(Trace<protocol::json::Sub> const &event) {
  profile_.sub([&]() {
    auto &[trace_info, sub] = event;
    if (sub.err_code != 0) {
      log::error(R"(Subscription failed: code={}, msg="{}")"sv, sub.err_code, sub.err_msg);
    }
  });
}

void DropCopy5::operator()(Trace<protocol::json::Response5> const &) {
  log::fatal("Unexpected"sv);
}
/*
void DropCopy5::operator()(Trace<protocol::json::FundingRate> const &) {
  log::fatal("Unexpected"sv);
}
*/
void DropCopy5::operator()(Trace<protocol::json::Account5> const &event) {
  profile_.accounts([&]() {
    auto &[trace_info, account] = event;
    log::info<2>("account={}"sv, account);
    auto update_type = map(account.event).template get<UpdateType>();
    for (auto &item : account.data.details) {
      auto funds_update = FundsUpdate{
          .stream_id = stream_id_,
          .account = account_.name,
          .currency = item.currency,
          .margin_mode = {},
          .balance = item.equity,
          .hold = NaN,
          .borrowed = NaN,
          .unrealized_pnl = NaN,
          .external_account = {},
          .update_type = update_type,
          .exchange_time_utc = {},
          .sending_time_utc = account.ts,
      };
      create_trace_and_dispatch(shared_.dispatcher, trace_info, funds_update, true);
    }
  });
}

void DropCopy5::operator()(Trace<protocol::json::Positions5> const &event) {
  profile_.positions([&]() {
    auto &[trace_info, positions] = event;
    auto update_type = map(positions.event).template get<UpdateType>();
    for (auto &item : positions.data) {
      auto direction = map(item.direction).template get<Side>();
      auto long_quantity = [&]() {
        if (direction == Side::BUY) {
          return item.available;  // ???
        }
        return NaN;
      }();
      auto short_quantity = [&]() {
        if (direction == Side::SELL) {
          return item.available;  // ???
        }
        return NaN;
      }();
      auto position_update = PositionUpdate{
          .stream_id = stream_id_,
          .account = account_.name,
          .exchange = shared_.settings.exchange,
          .symbol = item.contract_code,
          .margin_mode = map(item.margin_mode),
          .external_account = {},
          .long_quantity = long_quantity,
          .short_quantity = short_quantity,
          .update_type = update_type,
          .exchange_time_utc = {},
          .sending_time_utc = positions.ts,
      };
      create_trace_and_dispatch(shared_.dispatcher, trace_info, position_update, true);
    }
  });
}

// note!
// the order channel is much slower than match_orders
// if we allow match_orders, we only get the filled and the orders channel later catches up with the full sequence of new/filled
void DropCopy5::operator()(Trace<protocol::json::MatchOrders5> const &event) {
  profile_.match_orders([&]() {
    auto &[trace_info, match_orders] = event;
    log::debug("match_orders={}"sv, match_orders);
    for (auto &item : match_orders.data) {
      continue;  // note! DISABLED
      auto remaining_quantity = [&]() {
        if (utils::compare(item.volume, 0.0) > 0) {
          return item.volume - item.trade_volume;  // note! can't modify order
        }
        return NaN;
      }();
      auto order_update = server::oms::OrderUpdate{
          .account = account_.name,
          .exchange = shared_.settings.exchange,
          .symbol = item.contract_code,
          .side = map(item.side),
          .position_effect = map(item.position_side, item.side),
          .margin_mode = {},
          .max_show_quantity = NaN,
          .order_type = map(item.type),
          .time_in_force = {},
          .execution_instructions = {},
          .execution_destination = {},
          .create_time_utc = item.created_time,
          .update_time_utc = item.match_time,
          .external_account = {},
          .external_order_id = item.order_id,
          .client_order_id = item.client_order_id,
          .order_status = map(item.state),
          .error = {},
          .text = {},
          .quantity = item.volume,
          .price = item.price,
          .stop_price = NaN,
          .leverage = item.lever_rate,
          .remaining_quantity = remaining_quantity,
          .traded_quantity = item.trade_volume,
          .average_traded_price = NaN,
          .last_traded_quantity = NaN,
          .last_traded_price = NaN,
          .last_liquidity = {},
          .routing_id = {},
          .max_request_version = {},
          .max_response_version = {},
          .max_accepted_version = {},
          .update_type = UpdateType::INCREMENTAL,
          .sending_time_utc = match_orders.ts,
      };
      auto user_id = SOURCE_NONE;
      auto order_id = ORDER_ID_NONE;
      auto strategy_id = STRATEGY_ID_NONE;
      auto callback = [&](auto &order) {
        user_id = order.user_id;
        order_id = order.order_id;
        strategy_id = order.strategy_id;
      };
      create_trace_and_dispatch(shared_.dispatcher, trace_info, order_update, stream_id_, callback);
    }
  });
}

void DropCopy5::operator()(Trace<protocol::json::Orders5> const &event) {
  profile_.orders([&]() {
    auto &[trace_info, orders] = event;
    log::debug("orders={}"sv, orders);
    auto remaining_quantity = [&]() {
      if (utils::compare(orders.data.volume, 0.0) > 0) {
        return orders.data.volume - orders.data.trade_volume;  // note! can't modify order
      }
      return NaN;
    }();
    auto order_update = server::oms::OrderUpdate{
        .account = account_.name,
        .exchange = shared_.settings.exchange,
        .symbol = orders.data.contract_code,
        .side = map(orders.data.side),
        .position_effect = map(orders.data.position_side, orders.data.side),
        .margin_mode = {},
        .max_show_quantity = NaN,
        .order_type = map(orders.data.type),
        .time_in_force = {},
        .execution_instructions = {},
        .execution_destination = {},
        .create_time_utc = orders.data.created_time,
        .update_time_utc = orders.data.updated_time,
        .external_account = {},
        .external_order_id = orders.data.order_id,
        .client_order_id = orders.data.client_order_id,
        .order_status = map(orders.data.state),
        .error = {},
        .text = {},
        .quantity = orders.data.volume,
        .price = orders.data.price,
        .stop_price = NaN,
        .leverage = orders.data.lever_rate,
        .remaining_quantity = remaining_quantity,
        .traded_quantity = orders.data.trade_volume,
        .average_traded_price = NaN,
        .last_traded_quantity = NaN,
        .last_traded_price = NaN,
        .last_liquidity = {},
        .routing_id = {},
        .max_request_version = {},
        .max_response_version = {},
        .max_accepted_version = {},
        .update_type = UpdateType::INCREMENTAL,
        .sending_time_utc = orders.ts,
    };
    auto user_id = SOURCE_NONE;
    auto order_id = ORDER_ID_NONE;
    auto strategy_id = STRATEGY_ID_NONE;
    auto callback = [&](auto &order) {
      user_id = order.user_id;
      order_id = order.order_id;
      strategy_id = order.strategy_id;
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, order_update, stream_id_, callback);
    // XXX FIXME TODO trades
  });
}

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
