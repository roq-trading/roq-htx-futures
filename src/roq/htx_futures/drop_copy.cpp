/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/drop_copy.hpp"

#include "roq/mask.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/htx_futures/json/map.hpp"
#include "roq/htx_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {

// === CONSTANTS ===

namespace {
auto const NAME = "dc"sv;

auto const SUPPORTS = Mask{
    SupportType::FUNDS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 1;
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
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.ping_freq,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::socket::Client::create(handler, context, config, []() { return std::string(); });
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

DropCopy::DropCopy(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared)
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
      account_{account}, shared_{shared}, inflate_{core::zlib::Inflate::GZIP_NO_HEADER} {
}

void DropCopy::operator()(Event<Start> const &) {
  (*connection_).start();
}

void DropCopy::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void DropCopy::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
}

void DropCopy::operator()(metrics::Writer &writer) const {
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

void DropCopy::operator()(web::socket::Client::Connected const &) {
}

void DropCopy::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void DropCopy::operator()(web::socket::Client::Ready const &) {
  send_login();
  (*this)(ConnectionStatus::LOGIN_SENT);
}

void DropCopy::operator()(web::socket::Client::Close const &) {
}

void DropCopy::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.name,
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(web::socket::Client::Text const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(web::socket::Client::Binary const &binary) {
  if (inflate_.decode(binary.payload, inflate_buffer_, [&](auto &payload) {
        std::string_view message{reinterpret_cast<char const *>(std::data(payload)), std::size(payload)};
        log::info<5>(R"(message="{}")"sv, message);
        log::debug(R"(message="{}")"sv, message);
        parse(message);
      })) {
  } else {
    log::fatal("Failed to decode message"sv);
  }
}

void DropCopy::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = account_.name,
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::WS,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
        .interface = (*connection_).get_interface(),
        .authority = (*connection_).get_current_authority(),
        .path = (*connection_).get_current_path(),
        .proxy = (*connection_).get_proxy(),
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void DropCopy::send_pong(std::chrono::milliseconds timestamp) {
  auto message = fmt::format(
      R"({{)"
      R"("op":"pong",)"
      R"("ts":{})"
      R"(}})"sv,
      timestamp.count());
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void DropCopy::send_login() {
  auto now_utc = clock::get_realtime<std::chrono::seconds>();
  auto message = account_.create_ws_auth("/swap-notification"sv, now_utc);
  log::warn("DEBUG {}"sv, message);
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void DropCopy::subscribe() {
  subscribe("accounts.*"sv);
  subscribe("positions.*"sv);
  subscribe("matchOrders.*"sv);
  subscribe("orders.*"sv);
}

void DropCopy::subscribe(std::string_view const &topic) {
  auto message = fmt::format(
      R"({{)"
      R"("op":"sub",)"
      R"("topic":"{}",)"
      R"("cid":"xxx")"
      R"(}})"sv,
      topic);
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void DropCopy::parse(std::string_view const &message) {
  log::warn("DEBUG {}"sv, message);
  profile_.parse([&]() {
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    try {
      TraceInfo trace_info;
      if (!json::Parser2::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

void DropCopy::operator()(Trace<json::Close> const &) {
  profile_.close([&]() {
    log::warn("Exchange requested connection closed"sv);
    (*connection_).close();
  });
}

void DropCopy::operator()(Trace<json::Error2> const &) {
  profile_.error([&]() {
    log::warn("*** ERROR ***"sv);
    (*connection_).close();
  });
}

void DropCopy::operator()(Trace<json::Ping> const &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    send_pong(ping.timestamp);
  });
}

void DropCopy::operator()(Trace<json::Auth> const &event) {
  profile_.auth([&]() {
    auto &[trace_info, auth] = event;
    if (auth.err_code == 0) {
      subscribe();
      (*this)(ConnectionStatus::READY);
    } else {
      log::error(R"(Authentication failed: code={}, msg="{}")"sv, auth.err_code, auth.err_msg);
      (*connection_).close();
    }
  });
}

void DropCopy::operator()(Trace<json::Sub> const &event) {
  profile_.sub([&]() {
    auto &[trace_info, sub] = event;
    if (sub.err_code != 0) {
      log::error(R"(Subscription failed: code={}, msg="{}")"sv, sub.err_code, sub.err_msg);
    }
  });
}

void DropCopy::operator()(Trace<json::FundingRate> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Accounts> const &event) {
  profile_.accounts([&]() {
    auto &[trace_info, accounts] = event;
    auto update_type = map(accounts.event).template get<UpdateType>();
    for (auto &item : accounts.data) {
      auto funds_update = FundsUpdate{
          .stream_id = stream_id_,
          .account = account_.name,
          .currency = item.symbol,
          .margin_mode = {},
          .balance = item.margin_balance,
          .hold = NaN,
          .borrowed = NaN,
          .external_account = {},
          .update_type = update_type,
          .exchange_time_utc = {},
          .sending_time_utc = accounts.ts,
      };
      create_trace_and_dispatch(handler_, trace_info, funds_update, true);
    }
  });
}

void DropCopy::operator()(Trace<json::Positions> const &event) {
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
          .margin_mode = {},
          .external_account{},
          .long_quantity = long_quantity,
          .short_quantity = short_quantity,
          .update_type = update_type,
          .exchange_time_utc = {},
          .sending_time_utc = positions.ts,
      };
      create_trace_and_dispatch(handler_, trace_info, position_update, true);
    }
  });
}

void DropCopy::operator()(Trace<json::MatchOrders> const &event) {
  profile_.match_orders([&]() {
    auto &[trace_info, match_orders] = event;
    /*
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
          .margin_mode = {},
          .external_account{},
          .long_quantity = long_quantity,
          .short_quantity = short_quantity,
          .update_type = update_type,
          .exchange_time_utc = {},
          .sending_time_utc = positions.ts,
      };
      create_trace_and_dispatch(handler_, trace_info, position_update, true);
    }
    */
  });
}

void DropCopy::operator()(Trace<json::Orders> const &event) {
  profile_.orders([&]() {
    auto &[trace_info, orders] = event;
    /*
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
          .margin_mode = {},
          .external_account{},
          .long_quantity = long_quantity,
          .short_quantity = short_quantity,
          .update_type = update_type,
          .exchange_time_utc = {},
          .sending_time_utc = positions.ts,
      };
      create_trace_and_dispatch(handler_, trace_info, position_update, true);
    }
    */
  });
}

}  // namespace htx_futures
}  // namespace roq
