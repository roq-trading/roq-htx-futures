/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi_futures/order_entry.h"

#include <utility>

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/metrics/factory.h"

#include "roq/huobi_futures/flags.h"

#include "roq/huobi_futures/json/utils.h"

using namespace roq::literals;

namespace roq {
namespace huobi_futures {

namespace {
static const auto NAME = "om"_sv;
static const auto SUPPORTS = utils::Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
    SupportType::CREATE_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
    SupportType::FUNDS,
};

static const auto ALLOW_PIPELINING = true;

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

OrderEntry::OrderEntry(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared)
    : handler_(handler), stream_id_(stream_id),
      name_(fmt::format("{}:{}:{}"_sv, stream_id_, NAME, security.get_account())),
      connection_(
          *this,
          context,
          Flags::decode_buffer_size(),
          Flags::encode_buffer_size(),
          core::URI(Flags::rest_uri()),
          ROQ_PACKAGE_NAME,
          core::http::Connection::KEEP_ALIVE,
          ALLOW_PIPELINING,
          Flags::rest_request_timeout(),
          Flags::rest_rate_limit_interval(),
          Flags::rest_rate_limit_max_requests(),
          Flags::rest_ping_freq(),
          Flags::rest_ping_path()),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"_sv),
      },
      profile_{
          .listen_key = create_metrics(name_, "listen_key"_sv),
          .listen_key_ack = create_metrics(name_, "listen_key_ack"_sv),
          .account = create_metrics(name_, "account"_sv),
          .account_ack = create_metrics(name_, "account_ack"_sv),
          .exchange_info = create_metrics(name_, "exchange_info"_sv),
          .exchange_info_ack = create_metrics(name_, "exchange_info_ack"_sv),
          .new_order = create_metrics(name_, "new_order"_sv),
          .new_order_ack = create_metrics(name_, "new_order_ack"_sv),
          .cancel_order = create_metrics(name_, "cancel_order"_sv),
          .cancel_order_ack = create_metrics(name_, "cancel_order_ack"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
      },
      security_(security), shared_(shared),
      download_(Flags::rest_request_timeout(), [this](auto state) { return download(state); }) {
}

void OrderEntry::operator()(const Event<Start> &) {
  connection_.start();
}

void OrderEntry::operator()(const Event<Stop> &) {
  connection_.stop();
}

void OrderEntry::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
  refresh_listen_key();
}

void OrderEntry::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.listen_key, metrics::PROFILE)
      .write(profile_.listen_key_ack, metrics::PROFILE)
      .write(profile_.account, metrics::PROFILE)
      .write(profile_.account_ack, metrics::PROFILE)
      .write(profile_.exchange_info, metrics::PROFILE)
      .write(profile_.exchange_info_ack, metrics::PROFILE)
      .write(profile_.new_order, metrics::PROFILE)
      .write(profile_.new_order_ack, metrics::PROFILE)
      .write(profile_.cancel_order, metrics::PROFILE)
      .write(profile_.cancel_order_ack, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

uint16_t OrderEntry::operator()(
    const Event<CreateOrder> &event, const oms::Order &order, const std::string_view &request_id) {
  new_order(event, order, request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    const Event<ModifyOrder> &,
    const oms::Order &,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  throw oms::NotSupportedException();
}

uint16_t OrderEntry::operator()(
    const Event<CancelOrder> &event,
    const oms::Order &order,
    const std::string_view &request_id,
    const std::string_view &previous_request_id) {
  cancel_order(event, order, request_id, previous_request_id);
  return stream_id_;
}

uint16_t OrderEntry::operator()(
    const Event<CancelAllOrders> &, [[maybe_unused]] const std::string_view &request_id) {
  log::fatal("*** CANCEL ALL ORDERS *NOT* SUPPORTED ***"_sv);
}

void OrderEntry::operator()(const core::web::Client::Connected &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void OrderEntry::operator()(const core::web::Client::Disconnected &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void OrderEntry::operator()(const core::web::Client::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(trace_info, external_latency, handler_);
  latency_.ping.update(latency.sample);
}

void OrderEntry::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    server::TraceInfo trace_info;
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::REST,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"_sv, stream_status);
    server::create_trace_and_dispatch(trace_info, stream_status, handler_);
  }
}

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    case OrderEntryState::UNDEFINED:
      assert(false);
      break;
    case OrderEntryState::LISTEN_KEY:
      get_listen_key();
      return 1;
    case OrderEntryState::ACCOUNT:
      get_account();
      return 1;
    case OrderEntryState::EXCHANGE_INFO:
      get_exchange_info();
      return 1;
    case OrderEntryState::DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

// listen-key

void OrderEntry::get_listen_key() {
  profile_.listen_key([&]() {
    auto method = core::http::Method::POST;
    auto path = "/api/v3/userDataStream"_sv;
    auto headers = fmt::format("X-MBX-APIKEY: {}\r\n"_sv, security_.get_api_key());
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = {},
        .rate_limit_weight = 1,
    };
    auto sequence = download_.sequence();
    connection_(
        "listen_key"_sv,
        request,
        [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          server::TraceInfo trace_info;
          server::Trace event(trace_info, response);
          get_listen_key_ack(event, sequence);
        });
  });
}

void OrderEntry::get_listen_key_ack(
    const server::Trace<core::web::Response> &event, uint32_t sequence) {
  auto state = OrderEntryState::LISTEN_KEY;
  profile_.listen_key_ack([&]() {
    auto &[trace_info, response] = event;
    try {
      if (download_.skip(sequence, state))
        return;
      response.expect(core::http::Status::OK);
      auto body = response.body();
      auto listen_key = core::json::Parser::create<json::ListenKey>(body);
      server::Trace event(trace_info, listen_key);
      (*this)(event);
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(const server::Trace<json::ListenKey> &event) {
  auto &[trace_info, listen_key] = event;
  log::info<1>("listen_key={}"_sv, listen_key);
  bool initial = listen_key_.empty();
  if (utils::update(listen_key_, listen_key.listen_key)) {
    if (initial) {
      log::info(R"(Listen key has been acquired (value="{}"))"_sv, listen_key_);
      ListenKeyUpdate listen_key_update{
          .account = security_.get_account(),
          .listen_key = listen_key.listen_key,
      };
      create_trace_and_dispatch(trace_info, listen_key_update, handler_);
    } else {
      if (ROQ_UNLIKELY(!initial))
        log::info("Listen key has been refreshed!"_sv);
    }
  }
  auto now = core::get_system_clock();
  listen_key_refresh_ = now + Flags::rest_listen_key_refresh();
}

void OrderEntry::refresh_listen_key() {
  if (!ready())
    return;
  auto now = core::get_system_clock();
  if (listen_key_refresh_ == listen_key_refresh_.zero() || now < listen_key_refresh_)
    return;
  log::info("Refreshing listen key..."_sv);
  listen_key_refresh_ = now + Flags::rest_listen_key_refresh();
  get_listen_key();
  /*
  get<json::ListenKey>([this](auto &promise) {
    try {
      (*this)(promise.get());
    } catch (core::NetworkError &) {
      log::warn("Rescheduling listen key refresh!"_sv);
      auto now = core::get_system_clock();
      listen_key_refresh_ = now + Flags::rest_listen_key_refresh();
    }
  });
  */
}

// account

void OrderEntry::get_account() {
  profile_.account([&]() {
    auto method = core::http::Method::GET;
    auto path = "/api/v3/account"_sv;
    auto now = core::get_realtime_clock();
    auto [timestamp, signature] = security_.create_signature(now);
    auto query = fmt::format("?{}&signature={}"_sv, timestamp, signature);
    auto headers = fmt::format("X-MBX-APIKEY: {}\r\n"_sv, security_.get_api_key());
    core::web::Request request{
        .method = method,
        .path = path,
        .query = query,
        .accept = core::http::Accept::JSON,
        .content_type = {},
        .headers = headers,
        .body = {},
        .quality_of_service = {},
        .rate_limit_weight = 1,
    };
    auto sequence = download_.sequence();
    connection_(
        "account"_sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          server::TraceInfo trace_info;
          server::Trace event(trace_info, response);
          get_account_ack(event, sequence);
        });
  });
}

void OrderEntry::get_account_ack(
    const server::Trace<core::web::Response> &event, uint32_t sequence) {
  auto state = OrderEntryState::ACCOUNT;
  profile_.account_ack([&]() {
    auto &[trace_info, response] = event;
    try {
      if (download_.skip(sequence, state))
        return;
      response.expect(core::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      auto account = core::json::Parser::create<json::Account>(response.body(), buffer);
      server::Trace event(trace_info, account);
      (*this)(event);
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(const server::Trace<json::Account> &event) {
  auto &[trace_info, account] = event;
  log::info<1>("account={}"_sv, account);
  for (auto &item : account.balances) {
    FundsUpdate funds_update{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .currency = item.asset,
        .balance = item.free,
        .hold = item.locked,
        .external_account = {},
    };
    create_trace_and_dispatch(trace_info, funds_update, handler_, true);
  }
}

// exchange-info

void OrderEntry::get_exchange_info() {
  profile_.exchange_info([&]() {
    auto method = core::http::Method::GET;
    auto path = "/api/v3/exchangeInfo"_sv;
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
        .rate_limit_weight = 1,
    };
    auto sequence = download_.sequence();
    connection_(
        "exchange_info"_sv,
        request,
        [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          server::TraceInfo trace_info;
          server::Trace event(trace_info, response);
          get_exchange_info_ack(event, sequence);
        });
  });
}

void OrderEntry::get_exchange_info_ack(
    const server::Trace<core::web::Response> &event, uint32_t sequence) {
  auto state = OrderEntryState::EXCHANGE_INFO;
  profile_.exchange_info_ack([&]() {
    auto &[trace_info, response] = event;
    try {
      if (download_.skip(sequence, state))
        return;
      response.expect(core::http::Status::OK);
      auto body = response.body();
      core::json::Buffer buffer(decode_buffer_);
      auto exchange_info = core::json::Parser::create<json::ExchangeInfo>(body, buffer);
      server::Trace event(trace_info, exchange_info);
      (*this)(event);
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(const server::Trace<json::ExchangeInfo> &event) {
  auto &[trace_info, exchange_info] = event;
  log::info<1>("exchange_info={}"_sv, exchange_info);
  std::vector<std::string> symbols;
  size_t counter = {};
  for (const auto &item : exchange_info.symbols) {
    log::info<1>("item={}"_sv, item);
    if (shared_.discard_symbol(item.symbol)) {
      log::info<1>(R"(Drop symbol="{}")"_sv, item.symbol);
      continue;
    }
    // note! convert to lowercase
    std::string symbol(item.symbol);
    std::transform(
        symbol.begin(), symbol.end(), symbol.begin(), [](auto c) { return std::tolower(c); });
    if (all_symbols_.emplace(symbol).second)  // only include new
      symbols.emplace_back(symbol);
    ++counter;
    auto tick_size = std::pow(10.0, -static_cast<double>(item.quote_precision));
    auto min_trade_vol = std::pow(10.0, -static_cast<double>(item.base_asset_precision));
    ReferenceData reference_data{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = item.symbol,
        .description = {},
        .security_type = {},
        .base_currency = item.base_asset,
        .quote_currency = item.quote_asset,
        .commission_currency = {},
        .tick_size = tick_size,
        .multiplier = NaN,
        .min_trade_vol = min_trade_vol,
        .max_trade_vol = NaN,
        .trade_vol_step_size = min_trade_vol,
        .option_type = {},
        .strike_currency = {},
        .strike_price = NaN,
        .underlying = {},
        .time_zone = {},
        .issue_date = {},
        .settlement_date = {},
        .expiry_datetime = {},
        .expiry_datetime_utc = {},
    };
    create_trace_and_dispatch(trace_info, reference_data, handler_, false);
    auto trading_status = json::map(item.status);
    MarketStatus market_status{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = item.symbol,
        .trading_status = trading_status,
    };
    create_trace_and_dispatch(trace_info, market_status, handler_, true);
  }
  log::info("Exchange info: including symbols {}/{}"_sv, counter, exchange_info.symbols.size());
  if (!symbols.empty()) {
    SymbolsUpdate symbols_update{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
}

// new-order

void OrderEntry::new_order(
    const Event<CreateOrder> &event, const oms::Order &, const std::string_view &request_id) {
  profile_.new_order([&]() {
    auto &[trace_info, create_order] = event;
    if (!ready())
      throw oms::NotReadyException();
    auto method = core::http::Method::POST;
    auto path = "/api/v3/order"_sv;
    auto timestamp = core::get_realtime_clock();
    auto side = json::map(create_order.side).as_raw_text();
    auto type = json::map(create_order.order_type).as_raw_text();
    auto time_in_force = json::map(create_order.time_in_force).as_raw_text();
    // XXX use encode buffer
    auto body = fmt::format(
        R"({{)"
        R"("symbol":"{}",)"
        R"("side":"{}",)"
        R"("type":"{}",)"
        R"("timeInForce":"{}",)"
        R"("quantity":{},)"
        R"("quoteOrderQty":{},)"  // XXX ???
        R"("price":{},)"
        R"("newClientOrderId":"{}")"
        R"("stopPrice":{},)"   // XXX ???
        R"("icebergQty":{},)"  // XXX ???
        R"("recvWindow":{},)"
        R"("timestamp":{})"
        R"(}})"_sv,
        create_order.symbol,
        side,
        type,
        time_in_force,
        create_order.quantity,
        0.0,
        create_order.price,
        request_id,
        0.0,
        0.0,
        std::chrono::duration_cast<std::chrono::milliseconds>(Flags::rest_order_recv_window())
            .count(),
        timestamp.count());
    log::debug(R"(body="{}")"_sv, body);
    auto headers = fmt::format("X-MBX-APIKEY: {}\r\n"_sv, security_.get_api_key());
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = core::http::ContentType::JSON,
        .headers = headers,
        .body = body,
        .quality_of_service = core::web::QualityOfService::IMMEDIATE,
        .rate_limit_weight = 1,
    };
    connection_(request_id, request, [this]([[maybe_unused]] auto &request_id, auto &response) {
      server::TraceInfo trace_info;
      server::Trace event(trace_info, response);
      new_order_ack(event);
    });
  });
}

void OrderEntry::new_order_ack(const server::Trace<core::web::Response> &event) {
  profile_.new_order_ack([&]() {
    auto &[trace_info, response] = event;
    try {
      response.expect(core::http::Status::OK);
      auto body = response.body();
      core::json::Buffer buffer(decode_buffer_);
      auto new_order = core::json::Parser::create<json::NewOrder>(body, buffer);
      server::Trace event(trace_info, new_order);
      (*this)(event);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
      // XXX HANS ???
    }
  });
}

void OrderEntry::operator()(const server::Trace<json::NewOrder> &event) {
  auto &[trace_info, new_order] = event;
  log::info<1>("new_order={}"_sv, new_order);
  throw NotImplementedException();
}

// cancel-order

void OrderEntry::cancel_order(
    const Event<CancelOrder> &,
    const oms::Order &order,
    const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  profile_.cancel_order([&]() {
    if (!ready())
      throw oms::NotReadyException();
    auto method = core::http::Method::DELETE;
    auto path = "/api/v3/order"_sv;
    auto timestamp = core::get_realtime_clock();
    // XXX use encode buffer
    auto body = fmt::format(
        R"({{)"
        R"("symbol":"{}",)"
        R"("origClientOrderId":"{}")"
        R"("newClientOrderId":"{}")"
        R"("recvWindow":{},)"
        R"("timestamp":{})"
        R"(}})"_sv,
        order.symbol,
        order.external_order_id,
        request_id,
        std::chrono::duration_cast<std::chrono::milliseconds>(Flags::rest_order_recv_window())
            .count(),
        timestamp.count());
    log::debug(R"(body="{}")"_sv, body);
    auto headers = fmt::format("X-MBX-APIKEY: {}\r\n"_sv, security_.get_api_key());
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = core::http::ContentType::JSON,
        .headers = headers,
        .body = body,
        .quality_of_service = core::web::QualityOfService::IMMEDIATE,
        .rate_limit_weight = 1,
    };
    connection_(request_id, request, [this]([[maybe_unused]] auto &request_id, auto &response) {
      server::TraceInfo trace_info;
      server::Trace event(trace_info, response);
      cancel_order_ack(event);
    });
  });
}

void OrderEntry::cancel_order_ack(const server::Trace<core::web::Response> &event) {
  profile_.cancel_order_ack([&]() {
    auto &[trace_info, response] = event;
    try {
      response.expect(core::http::Status::OK);
      auto body = response.body();
      auto cancel_order = core::json::Parser::create<json::CancelOrder>(body);
      server::Trace event(trace_info, cancel_order);
      (*this)(event);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
      // XXX HANS ???
    }
  });
}

void OrderEntry::operator()(const server::Trace<json::CancelOrder> &event) {
  auto &[trace_info, cancel_order] = event;
  log::info<1>("cancel_order={}"_sv, cancel_order);
  throw NotImplementedException();
}

}  // namespace huobi_futures
}  // namespace roq
