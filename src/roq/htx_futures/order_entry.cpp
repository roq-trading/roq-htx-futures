/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/order_entry.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/htx_futures/json/encoder.hpp"
#include "roq/htx_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {

// === CONSTANTS ===

namespace {
auto const NAME = "om"sv;

auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
    SupportType::CREATE_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
    SupportType::FUNDS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 2;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id, auto const &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.rest.uri;
  auto config = web::rest::Client::Config{
      // connection
      .interface = {},
      .proxy = settings.rest.proxy,
      .uris = {&uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = {},
      .disconnect_on_idle_timeout = {},
      .connection = web::http::Connection::KEEP_ALIVE,
      // request
      .allow_pipelining = true,
      .request_timeout = settings.rest.request_timeout,
      // response
      .suspend_on_retry_after = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .ping_frequency = settings.rest.ping_freq,
      .ping_path = settings.rest.ping_path,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::rest::Client::create(handler, context, config);
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

OrderEntry::OrderEntry(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account.name)}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .account_info = create_metrics(shared.settings, name_, "account_info"sv),
          .account_info_ack = create_metrics(shared.settings, name_, "account_info_ack"sv),
          .create_order = create_metrics(shared.settings, name_, "create_order"sv),
          .create_order_ack = create_metrics(shared.settings, name_, "create_order_ack"sv),
          .cancel_order = create_metrics(shared.settings, name_, "cancel_order"sv),
          .cancel_order_ack = create_metrics(shared.settings, name_, "cancel_order_ack"sv),
          .cancel_all_orders = create_metrics(shared.settings, name_, "cancel_all_orders"sv),
          .cancel_all_orders_ack = create_metrics(shared.settings, name_, "cancel_all_orders_ack"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      account_{account}, shared_{shared}, download_{shared.settings.rest.request_timeout, [this](auto state) { return download(state); }} {
}

void OrderEntry::operator()(Event<Start> const &) {
  (*connection_).start();
}

void OrderEntry::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void OrderEntry::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
}

void OrderEntry::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.account_info, metrics::Type::PROFILE)
      .write(profile_.account_info_ack, metrics::Type::PROFILE)
      .write(profile_.create_order, metrics::Type::PROFILE)
      .write(profile_.create_order_ack, metrics::Type::PROFILE)
      .write(profile_.cancel_order, metrics::Type::PROFILE)
      .write(profile_.cancel_order_ack, metrics::Type::PROFILE)
      .write(profile_.cancel_all_orders, metrics::Type::PROFILE)
      .write(profile_.cancel_all_orders_ack, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

uint16_t OrderEntry::operator()(Event<CreateOrder> const &, server::oms::Order const &, [[maybe_unused]] std::string_view const &request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t OrderEntry::operator()(
    Event<ModifyOrder> const &,
    server::oms::Order const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t OrderEntry::operator()(
    Event<CancelOrder> const &,
    server::oms::Order const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t OrderEntry::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  cancel_all_orders(event, request_id);
  return stream_id_;
}

void OrderEntry::operator()(Trace<web::rest::Client::Connected> const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void OrderEntry::operator()(Trace<web::rest::Client::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading()) {
    download_.reset();
  }
}

void OrderEntry::operator()(Trace<web::rest::Client::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.name,
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void OrderEntry::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = account_.name,
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::HTTP,
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

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    using enum OrderEntryState;
    case UNDEFINED:
      assert(false);
      break;
    case ACCOUNT_INFO_ISOLATED:
      get_account_info_isolated();
      return 1;
    case ACCOUNT_INFO_CROSS:
      get_account_info_cross();
      return 1;
    case DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

// account-info

void OrderEntry::get_account_info_isolated() {
  profile_.account_info([&]() {
    auto method = web::http::Method::POST;
    auto path = "/linear-swap-api/v1/swap_account_info"sv;
    auto now_utc = clock::get_realtime<std::chrono::seconds>();
    auto query = account_.create_query(method, path, now_utc);
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence(), margin_mode = MarginMode::ISOLATED]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_account_info_ack(event, sequence, margin_mode);
    };
    (*connection_)("account-info-isolated"sv, request, callback);
  });
}

void OrderEntry::get_account_info_cross() {
  profile_.account_info([&]() {
    auto method = web::http::Method::POST;
    auto path = "/linear-swap-api/v1/swap_cross_account_info"sv;
    auto now_utc = clock::get_realtime<std::chrono::seconds>();
    auto query = account_.create_query(method, path, now_utc);
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = query,
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence(), margin_mode = MarginMode::CROSS]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_account_info_ack(event, sequence, margin_mode);
    };
    (*connection_)("account-info-cross"sv, request, callback);
  });
}

void OrderEntry::get_account_info_ack(Trace<web::rest::Response> const &event, [[maybe_unused]] uint32_t sequence, MarginMode margin_mode) {
  profile_.account_info_ack([&]() {
    auto const STATE = [&]() {
      switch (margin_mode) {
        using enum MarginMode;
        case UNDEFINED:
          break;
        case ISOLATED:
          return OrderEntryState::ACCOUNT_INFO_ISOLATED;
        case CROSS:
          return OrderEntryState::ACCOUNT_INFO_CROSS;
        case PORTFOLIO:
          break;
      }
      log::fatal("Unexpected"sv);
    }();
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(Downloading account-info FAILED: origin={}, error={}, status={}, text="{}")"sv, origin, error, status, text);
      if (download_.downloading()) {
        download_.retry(STATE);
      }
    };
    auto handle_success = [&](auto &body) {
      log::warn(R"(DEBUG body="{}")"sv, body);
      json::AccountInfo account_info{body, decode_buffer_};  // XXX FIXME TODO maybe it's different based on margin_mode? (used cross for testing)
      if (account_info.status == "ok"sv) {
        Trace event_2{event, account_info};
        (*this)(event_2, margin_mode);
        download_.check_relaxed(STATE);
        log::warn("Downloading account-info COMPLETED!"sv);
      } else {
        // handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, json::map_error(account_info.status_code), ""sv);
      }
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntry::operator()(Trace<json::AccountInfo> const &event, MarginMode margin_mode) {
  auto &[trace_info, account_info] = event;
  log::info<2>("account_info={}"sv, account_info);
  log::info("DEBUG margin_mode={}"sv, margin_mode);
  log::info("DEBUG account_info={}"sv, account_info);
}

// cancel-all-orders

void OrderEntry::cancel_all_orders(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  profile_.cancel_all_orders([&]() {
    if (!ready()) [[unlikely]] {
      throw server::oms::NotReady{"not ready"sv};
    }
    auto &[message_info, cancel_all_orders] = event;
    auto send_ack = [&]() {
      auto cancel_all_orders_ack = CancelAllOrdersAck{
          .stream_id = stream_id_,
          .account = account_.name,
          .order_id = cancel_all_orders.order_id,
          .exchange = cancel_all_orders.exchange,
          .symbol = cancel_all_orders.symbol,
          .side = cancel_all_orders.side,
          .origin = Origin::GATEWAY,
          .request_status = RequestStatus::FORWARDED,
          .error = {},
          .text = {},
          .request_id = request_id,
          .external_account = {},
          .number_of_affected_orders = {},
          .round_trip_latency = {},
          .user = {},
          .strategy_id = cancel_all_orders.strategy_id,
      };
      TraceInfo trace_info{event};
      Trace event_2{trace_info, cancel_all_orders_ack};
      shared_(event_2);
    };
    auto method = web::http::Method::POST;
    auto path = shared_.api.cancel_all_orders;
    auto body = json::Encoder::cancel_all_orders(encode_buffer_, cancel_all_orders, request_id);
    log::info<2>(R"(body="{}")"sv, body);
    log::warn(R"(DEBUG body="{}")"sv, body);
    auto request = web::rest::Request{
        .method = method,
        .path = path,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = web::http::ContentType::APPLICATION_JSON,
        .headers = {},
        .body = body,
        .quality_of_service = {},
    };
    auto callback = [this](auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      cancel_all_orders_ack(event, request_id);
    };
    (*connection_)(request_id, request, callback);
    send_ack();
  });
}

void OrderEntry::cancel_all_orders_ack(Trace<web::rest::Response> const &event, std::string_view const &request_id) {
  profile_.cancel_all_orders_ack([&]() {
    auto send_ack = [&](auto origin, auto status, Error error, std::string_view const &text) {
      auto cancel_all_orders_ack = CancelAllOrdersAck{
          .stream_id = stream_id_,
          .account = account_.name,
          .order_id = {},
          .exchange = {},
          .symbol = {},
          .side = {},
          .origin = origin,
          .request_status = status,
          .error = error,
          .text = text,
          .request_id = request_id,
          .external_account = {},
          .number_of_affected_orders = {},
          .round_trip_latency = {},
          .user = {},
          .strategy_id = {},
      };
      Trace event_2{event, cancel_all_orders_ack};
      shared_(event_2);
    };
    auto handle_error = [&](auto origin, auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      send_ack(origin, status, error, text);
    };
    auto handle_success = [&](auto &body) {
      log::warn(R"(DEBUG body="{}")"sv, body);
      /*
      json::CancelAllOrdersAck cancel_all_orders_ack{body, decode_buffer_};
      for (auto &item : cancel_all_orders_ack.data) {
        OrderUpdate{shared_, stream_id_, account_.name}(item, event.trace_info, false);
      }
      send_ack(Origin::EXCHANGE, RequestStatus::ACCEPTED, {}, {});
      */
    };
    process_response(event, handle_error, handle_success);
  });
}

void OrderEntry::process_response(web::rest::Response const &response, auto error_handler, auto success_handler) {
  try {
    auto [status, category, body] = response.result();
    switch (category) {
      using enum web::http::Category;
      case UNKNOWN:
      case INFORMATIONAL_RESPONSE:
        response.expect(web::http::Status::OK);  // throws
        break;
      case SUCCESS:
        success_handler(body);
        break;
      case REDIRECTION:
        log::fatal("Unexpected: URL is being redirected"sv);
      case CLIENT_ERROR:
        switch (status) {
          using enum web::http::Status;
          case FORBIDDEN:            // 403
          case I_AM_A_TEAPOT:        // 418
          case TOO_MANY_REQUESTS: {  // 429
            auto text = fmt::format("{}"sv, status);
            error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::REQUEST_RATE_LIMIT_REACHED, text);
            break;
          }
          case CONFLICT:  // 409
            assert(false);
            [[fallthrough]];
          default:
            error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, ""sv);
        }
        break;
      case SERVER_ERROR: {
        auto text = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, text);
        break;
      }
    }
  } catch (server::oms::Exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(e.origin, e.status, e.error, e.what());
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::GATEWAY, e.request_status(), e.error(), e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, e.what());
  }
}

template <typename... Args>
void OrderEntry::operator()(Trace<server::oms::Response> const &event, uint8_t user_id, uint64_t order_id, Args &&...args) {
  auto &[trace_info, response] = event;
  if (shared_.update_order(user_id, order_id, stream_id_, trace_info, response, std::forward<Args>(args)..., []([[maybe_unused]] auto &order) {})) {
  } else {
    log::warn("Did not find order: user_id={}, order_id={}"sv, user_id, order_id);
  }
}

void OrderEntry::operator()(Trace<server::oms::OrderUpdate> const &event, std::string_view const &client_order_id) {
  auto &[trace_info, order_update] = event;
  if (shared_.update_order(client_order_id, stream_id_, trace_info, order_update, [&]([[maybe_unused]] auto &order) {})) {
  } else {
    log::warn("*** EXTERNAL ORDER ***"sv);
  }
}
}  // namespace htx_futures
}  // namespace roq
