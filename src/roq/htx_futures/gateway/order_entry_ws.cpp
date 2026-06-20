/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/gateway/order_entry_ws.hpp"

#include "roq/mask.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/htx_futures/protocol/json/encoder.hpp"
#include "roq/htx_futures/protocol/json/map.hpp"
#include "roq/htx_futures/protocol/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "om"sv;

auto const SUPPORTS = Mask{
    SupportType::CREATE_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 1;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.ws.order2_uri;
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
  return settings.ws.order2_uri.get_path();
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

OrderEntryWS::OrderEntryWS(OrderEntry::Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared)
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
          .create_order = create_metrics(shared.settings, name_, "create_order"sv),
          .cancel_order = create_metrics(shared.settings, name_, "cancel_order"sv),
          .cancel_all_orders = create_metrics(shared.settings, name_, "cancel_all_orders"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      account_{account}, auth_path_{create_auth_path(shared.settings)}, shared_{shared}, inflate_{core::zlib::Inflate::GZIP_NO_HEADER} {
}

void OrderEntryWS::operator()(Event<Start> const &) {
  (*connection_).start();
}

void OrderEntryWS::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void OrderEntryWS::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
}

void OrderEntryWS::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      .write(profile_.close, metrics::Type::PROFILE)
      .write(profile_.error, metrics::Type::PROFILE)
      .write(profile_.ping, metrics::Type::PROFILE)
      .write(profile_.auth, metrics::Type::PROFILE)
      .write(profile_.create_order, metrics::Type::PROFILE)
      .write(profile_.cancel_order, metrics::Type::PROFILE)
      .write(profile_.cancel_all_orders, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

uint16_t OrderEntryWS::operator()(
    Event<CreateOrder> const &event, server::oms::Order const &order, server::oms::RefData const &ref_data, std::string_view const &request_id) {
  profile_.create_order([&]() {
    auto &[message_info, create_order] = event;
    auto message = protocol::json::Encoder::create_order_ws(encode_buffer_, create_order, order, ref_data, request_id, account_.margin_mode);
    log::debug_info<2>(R"(message="{}")"sv, message);
    (*connection_).send_text(message);
  });
  return stream_id_;
}

uint16_t OrderEntryWS::operator()(
    Event<ModifyOrder> const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw server::oms::NotSupported{"not supported"sv};
  return stream_id_;
}

uint16_t OrderEntryWS::operator()(
    Event<CancelOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  profile_.cancel_order([&]() {
    auto &[message_info, cancel_order] = event;
    auto message =
        protocol::json::Encoder::cancel_order_ws(encode_buffer_, cancel_order, order, ref_data, request_id, previous_request_id, account_.margin_mode);
    log::debug_info<2>(R"(message="{}")"sv, message);
    (*connection_).send_text(message);
  });
  return stream_id_;
}

uint16_t OrderEntryWS::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  profile_.cancel_all_orders([&]() {
    auto &[message_info, cancel_all_orders] = event;
    auto helper = [&](auto &symbol) {
      auto message = protocol::json::Encoder::cancel_all_orders_ws(encode_buffer_, cancel_all_orders, request_id, symbol, account_.margin_mode);
      log::debug_info<2>(R"(message="{}")"sv, message);
      (*connection_).send_text(message);
    };
    if (shared_.dispatcher.get_all_order_symbols(helper, account_.name)) {
    } else {
      log::warn("*** NOT POSSIBLE TO CANCEL ALL OPEN ORDERS (NO SYMBOLS) ***"sv);
    }
  });
  return stream_id_;
}

// web::socket::Client::Handler

void OrderEntryWS::operator()(web::socket::Client::Connected const &) {
}

void OrderEntryWS::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void OrderEntryWS::operator()(web::socket::Client::Ready const &) {
  send_login();
  // (*this)(ConnectionStatus::LOGIN_SENT);
  (*this)(ConnectionStatus::READY);
}

void OrderEntryWS::operator()(web::socket::Client::Close const &) {
}

void OrderEntryWS::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.name,
      .latency = latency.sample,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void OrderEntryWS::operator()(web::socket::Client::Text const &) {
  log::fatal("Unexpected"sv);
}

void OrderEntryWS::operator()(web::socket::Client::Binary const &binary) {
  if (inflate_.decode(binary.payload, inflate_buffer_, [&](auto &payload) {
        std::string_view message{reinterpret_cast<char const *>(std::data(payload)), std::size(payload)};
        log::info<5>(R"(message="{}")"sv, message);
        parse(message);
      })) {
  } else {
    log::fatal("Failed to decode message"sv);
  }
}

void OrderEntryWS::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
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

void OrderEntryWS::send_pong(std::chrono::milliseconds timestamp) {
  auto message = fmt::format(
      R"({{)"
      R"("op":"pong",)"
      R"("ts":{})"
      R"(}})"sv,
      timestamp.count());
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void OrderEntryWS::send_login() {
  auto now_utc = clock::get_realtime<std::chrono::seconds>();
  auto message = account_.create_ws_auth(auth_path_, now_utc);
  // log::debug(R"(message="{}")"sv, message);
  (*connection_).send_text(message);
}

void OrderEntryWS::parse(std::string_view const &message) {
  // log::debug("{}"sv, message);
  profile_.parse([&]() {
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    try {
      TraceInfo trace_info;
      if (!protocol::json::Parser3::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

// protocol::json::Parser3::Handler

void OrderEntryWS::operator()(Trace<protocol::json::Close2> const &) {
  profile_.close([&]() {
    log::warn("Exchange requested connection closed"sv);
    (*connection_).close();
  });
}

void OrderEntryWS::operator()(Trace<protocol::json::Error2> const &event) {
  profile_.error([&]() {
    auto &[trace_info, error] = event;
    log::error("error={}"sv, error);
  });
}

void OrderEntryWS::operator()(Trace<protocol::json::Ping> const &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    send_pong(ping.timestamp);
  });
}

void OrderEntryWS::operator()(Trace<protocol::json::Auth> const &event) {
  profile_.auth([&]() {
    auto &[trace_info, auth] = event;
    if (auth.err_code == 0) {
      (*this)(ConnectionStatus::READY);
    } else {
      log::error(R"(Authentication failed: code={}, msg="{}")"sv, auth.err_code, auth.err_msg);
      (*connection_).close();
    }
  });
}

void OrderEntryWS::operator()(Trace<protocol::json::Response> const &event) {
  auto &[trace_info, response] = event;
  log::info<2>("response={}"sv, response);
  auto [request_type, request_id, version] = protocol::json::Encoder::split_cid(response.cid);
  log::info<4>(R"(request_type={}, request_id="{}", version={})"sv, request_type, request_id, version);
  if (request_type == RequestType::UNDEFINED) {  // note! cancel-all-orders
    return;
  }
  auto [request_status, error, text] = [&]() -> std::tuple<RequestStatus, Error, std::string_view> {
    if (response.status == protocol::json::Status::OK) {
      return {RequestStatus::ACCEPTED, {}, {}};
    }
    if (std::empty(response.data.errors)) {
      return {RequestStatus::REJECTED, protocol::json::guess_error(response.err_code), response.err_msg};
    }
    if (std::size(response.data.errors) == 1) {
      auto &error = response.data.errors[0];
      return {RequestStatus::REJECTED, protocol::json::guess_error(error.err_code), error.err_msg};
    }
    log::fatal("Unexpected: response={}"sv, response);  // note! more errors, why?
  }();
  auto response_2 = server::oms::Response{
      .request_type = request_type,
      .origin = Origin::EXCHANGE,
      .request_status = request_status,
      .error = error,
      .text = text,
      .version = version,
      .request_id = request_id,
      .external_order_id = response.data.order_id_str,
      .client_order_id = {},
      .quantity = NaN,
      .price = NaN,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, response_2, stream_id_);
}

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
