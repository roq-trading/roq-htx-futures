/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi/order_entry.h"

#include <utility>

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/metrics/factory.h"

#include "roq/huobi/flags.h"

#include "roq/huobi/json/utils.h"

using namespace roq::literals;

namespace roq {
namespace huobi {

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

static const auto KEEP_ALIVE = true;
static const auto ALLOW_PIPELINING = true;

static const auto ACCEPT_ALL = "*/*"_sv;
static const auto ACCEPT_JSON = "application/json"_sv;
static const auto CONTENT_TYPE_JSON = "application/json"_sv;

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
      name_(roq::format("{}:{}:{}"_fmt, stream_id_, NAME, security.get_account())),
      connection_(
          *this,
          context,
          Flags::decode_buffer_size(),
          Flags::encode_buffer_size(),
          core::URI(Flags::rest_uri()),
          ROQ_PACKAGE_NAME,
          KEEP_ALIVE,
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
          .exchange_info = create_metrics(name_, "exchange_info"_sv),
          .account = create_metrics(name_, "account"_sv),
          .listen_key = create_metrics(name_, "listen_key"_sv),
          .depth = create_metrics(name_, "depth"_sv),
          .new_order = create_metrics(name_, "new_order"_sv),
          .cancel_order = create_metrics(name_, "cancel_order"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
      },
      security_(security), shared_(shared),
      download_(Flags::rest_request_timeout(), [this](auto state) { return download(state); }) {
}

bool OrderEntry::ready() const {
  return connection_.ready();
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
      .write(profile_.exchange_info, metrics::PROFILE)
      .write(profile_.account, metrics::PROFILE)
      .write(profile_.listen_key, metrics::PROFILE)
      .write(profile_.depth, metrics::PROFILE)
      .write(profile_.new_order, metrics::PROFILE)
      .write(profile_.cancel_order, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

void OrderEntry::operator()(
    const Event<CreateOrder> &event,
    const std::string_view &request_id,
    [[maybe_unused]] uint32_t gateway_order_id) {
  create_order(event.value, request_id, [this](auto &promise) {
    try {
      (*this)(promise.get());
    } catch (NetworkError &e) {
      // XXX send ack failure
      log::fatal(R"(Unexpected what="{}")"_fmt, e.what());
    }
  });
}

void OrderEntry::operator()(
    const Event<ModifyOrder> &, const std::string_view &, const server::OMS_Order &order) {
  throw server::OMS_Exception(Error::MODIFY_ORDER_NOT_SUPPORTED, order);
}

void OrderEntry::operator()(
    const Event<CancelOrder> &event,
    const std::string_view &request_id,
    const server::OMS_Order &order) {
  cancel_order(event.value, request_id, order, [this](auto &promise) {
    try {
      (*this)(promise.get());
    } catch (NetworkError &e) {
      // XXX send ack failure
      log::fatal(R"(Unexpected what="{}")"_fmt, e.what());
    }
  });
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
  ready_ = false;
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
    log::info("stream_status={}"_fmt, stream_status);
    server::create_trace_and_dispatch(trace_info, stream_status, handler_);
  }
}

template <>
void OrderEntry::get(std::function<void(const core::Promise<json::ExchangeInfo> &)> &&callback) {
  constexpr auto method = core::http::Method::GET;
  constexpr std::string_view path = "/api/v3/exchangeInfo"_sv;
  connection_.request(
      method,
      path,
      {},  // query
      ACCEPT_ALL,
      {},  // content_type
      {},  // headers
      {},  // body
      {},  // QoS
      [this, callback{std::move(callback)}](auto &response) {
        profile_.exchange_info([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            auto exchange_info =
                core::json::Parser::create<json::ExchangeInfo>(response.body(), buffer);
            log::trace_1("exchange_info={}"_fmt, exchange_info);
            core::Promise<json::ExchangeInfo> promise(exchange_info);
            callback(promise);
          } catch (NetworkError &e) {
            log::warn(R"(Exception type={}, what="{}")"_fmt, typeid(e).name(), e.what());
            core::Promise<json::ExchangeInfo> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

template <>
void OrderEntry::get(std::function<void(const core::Promise<json::Account> &)> &&callback) {
  constexpr auto method = core::http::Method::GET;
  constexpr std::string_view path = "/api/v3/account"_sv;
  auto now = core::get_realtime_clock();
  auto [timestamp, signature] = security_.create_signature(now);
  auto query = roq::format("?{}&signature={}"_fmt, timestamp, signature);
  auto headers = roq::format("X-MBX-APIKEY: {}\r\n"_fmt, security_.get_api_key());
  connection_.request(
      method,
      path,
      query,
      ACCEPT_ALL,
      {},  // content_type
      headers,
      {},  // body
      {},  // QoS
      [this, callback{std::move(callback)}](auto &response) {
        profile_.account([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            auto account = core::json::Parser::create<json::Account>(response.body(), buffer);
            log::trace_1("account={}"_fmt, account);
            core::Promise<json::Account> promise(account);
            callback(promise);
          } catch (NetworkError &e) {
            log::warn(R"(Exception type={}, what="{}")"_fmt, typeid(e).name(), e.what());
            core::Promise<json::Account> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

template <>
void OrderEntry::get(std::function<void(const core::Promise<json::ListenKey> &)> &&callback) {
  constexpr auto method = core::http::Method::POST;
  constexpr std::string_view path = "/api/v3/userDataStream"_sv;
  auto headers = roq::format("X-MBX-APIKEY: {}\r\n"_fmt, security_.get_api_key());
  connection_.request(
      method,
      path,
      {},  // query
      ACCEPT_ALL,
      {},  // content_type
      headers,
      {},  // body
      {},  // QoS
      [this, callback{std::move(callback)}](auto &response) {
        profile_.listen_key([&]() {
          try {
            response.expect(core::http::Status::OK);
            auto listen_key = core::json::Parser::create<json::ListenKey>(response.body());
            log::trace_1("listen_key={}"_fmt, listen_key);
            core::Promise<json::ListenKey> promise(listen_key);
            callback(promise);
          } catch (NetworkError &e) {
            log::warn(R"(Exception type={}, what="{}")"_fmt, typeid(e).name(), e.what());
            core::Promise<json::ListenKey> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    case OrderEntryState::UNDEFINED:
      assert(false);
      break;
    case OrderEntryState::LISTEN_KEY:
      download_listen_key();
      return 1u;
    case OrderEntryState::ACCOUNT:
      download_account();
      return 1u;
    case OrderEntryState::EXCHANGE_INFO:
      download_exchange_info();
      return 1u;
    case OrderEntryState::DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return {};
  }
  assert(false);
  return {};
}

void OrderEntry::download_listen_key() {
  constexpr auto state = OrderEntryState::LISTEN_KEY;
  auto sequence = download_.sequence();
  get<json::ListenKey>([this, sequence](auto &promise) {
    try {
      if (download_.skip(sequence, state))
        return;
      (*this)(promise.get());
      download_.check(state);
    } catch (NetworkError &) {
      download_.retry(state);
    }
  });
}

void OrderEntry::download_account() {
  constexpr auto state = OrderEntryState::ACCOUNT;
  auto sequence = download_.sequence();
  get<json::Account>([this, sequence](auto &promise) {
    try {
      if (download_.skip(sequence, state))
        return;
      (*this)(promise.get());
      download_.check(state);
    } catch (NetworkError &) {
      download_.retry(state);
    }
  });
}

void OrderEntry::download_exchange_info() {
  constexpr auto state = OrderEntryState::EXCHANGE_INFO;
  auto sequence = download_.sequence();
  get<json::ExchangeInfo>([this, sequence](auto &promise) {
    try {
      if (download_.skip(sequence, state))
        return;
      (*this)(promise.get());
      download_.check(state);
    } catch (NetworkError &) {
      download_.retry(state);
    }
  });
}

void OrderEntry::refresh_listen_key() {
  if (!ready_)
    return;
  auto now = core::get_system_clock();
  if (listen_key_refresh_ == listen_key_refresh_.zero() || now < listen_key_refresh_)
    return;
  log::info("Refreshing listen key..."_sv);
  listen_key_refresh_ = now + Flags::rest_listen_key_refresh();
  get<json::ListenKey>([this](auto &promise) {
    try {
      (*this)(promise.get());
    } catch (NetworkError &) {
      log::warn("Rescheduling listen key refresh!"_sv);
      auto now = core::get_system_clock();
      listen_key_refresh_ = now + Flags::rest_listen_key_refresh();
    }
  });
}

void OrderEntry::create_order(
    const CreateOrder &create_order,
    const std::string_view &cl_ord_id,
    std::function<void(const core::Promise<json::NewOrder> &)> &&callback) {
  auto method = core::http::Method::POST;
  auto path = "/api/v3/order"_sv;
  auto timestamp = core::get_realtime_clock();
  auto side = json::map(create_order.side).as_raw_text();
  auto type = json::map(create_order.order_type).as_raw_text();
  auto time_in_force = json::map(create_order.time_in_force).as_raw_text();
  // XXX use encode buffer
  auto message = roq::format(
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
      R"(}})"_fmt,
      create_order.symbol,
      side,
      type,
      time_in_force,
      create_order.quantity,
      0.0,
      create_order.price,
      cl_ord_id,
      0.0,
      0.0,
      Flags::rest_order_recv_window().count(),
      timestamp.count());
  log::debug(R"(body="{}")"_fmt, message);
  auto headers = roq::format("X-MBX-APIKEY: {}\r\n"_fmt, security_.get_api_key());
  connection_.request(
      method,
      path,
      {},  // query
      ACCEPT_JSON,
      CONTENT_TYPE_JSON,
      headers,
      {},  // body
      core::web::QualityOfService::IMMEDIATE,
      [this, callback{std::move(callback)}](auto &response) {
        profile_.new_order([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            auto new_order = core::json::Parser::create<json::NewOrder>(response.body(), buffer);
            log::trace_1("new_order={}"_fmt, new_order);
            core::Promise<json::NewOrder> promise(new_order);
            callback(promise);
          } catch (NetworkError &e) {
            log::warn(R"(Exception type={}, what="{}")"_fmt, typeid(e).name(), e.what());
            core::Promise<json::NewOrder> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

void OrderEntry::cancel_order(
    [[maybe_unused]] const CancelOrder &cancel_order,
    const std::string_view &request_id,
    const server::OMS_Order &order,
    std::function<void(const core::Promise<json::CancelOrder> &)> &&callback) {
  auto method = core::http::Method::DELETE;
  auto path = "/api/v3/order"_sv;
  auto timestamp = core::get_realtime_clock();
  // XXX use encode buffer
  auto message = roq::format(
      R"({{)"
      R"("symbol":"{}",)"
      R"("origClientOrderId":"{}")"
      R"("newClientOrderId":"{}")"
      R"("recvWindow":{},)"
      R"("timestamp":{})"
      R"(}})"_fmt,
      order.symbol,
      order.external_order_id,
      request_id,
      Flags::rest_order_recv_window().count(),
      timestamp.count());
  log::debug(R"(body="{}")"_fmt, message);
  auto headers = roq::format("X-MBX-APIKEY: {}\r\n"_fmt, security_.get_api_key());
  connection_.request(
      method,
      path,
      {},  // query
      ACCEPT_JSON,
      CONTENT_TYPE_JSON,
      headers,
      {},  // body
      core::web::QualityOfService::IMMEDIATE,
      [this, callback{std::move(callback)}](auto &response) {
        profile_.cancel_order([&]() {
          try {
            response.expect(core::http::Status::OK);
            auto cancel_order = core::json::Parser::create<json::CancelOrder>(response.body());
            log::trace_1("cancel_order={}"_fmt, cancel_order);
            core::Promise<json::CancelOrder> promise(cancel_order);
            callback(promise);
          } catch (NetworkError &e) {
            log::warn(R"(Exception type={}, what="{}")"_fmt, typeid(e).name(), e.what());
            core::Promise<json::CancelOrder> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

void OrderEntry::operator()(const json::NewOrder &) {
  log::fatal("NOT IMPLEMENTED"_sv);
}

void OrderEntry::operator()(const json::CancelOrder &) {
  log::fatal("NOT IMPLEMENTED"_sv);
}

void OrderEntry::operator()(const json::ListenKey &listen_key) {
  server::TraceInfo trace_info;  // note! not correct (*after* message parsing)
  bool initial = listen_key_.empty();
  if (utils::update(listen_key_, listen_key.listen_key)) {
    if (initial) {
      log::info(R"(Listen key has been acquired (value="{}"))"_fmt, listen_key_);
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

void OrderEntry::operator()(const json::Account &account) {
  server::TraceInfo trace_info;  // note! not correct (*after* message parsing)
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

void OrderEntry::operator()(const json::ExchangeInfo &exchange_info) {
  server::TraceInfo trace_info;  // note! not correct (*after* message parsing)
  std::vector<std::string> symbols;
  size_t counter = {};
  for (const auto &item : exchange_info.symbols) {
    log::trace_1("item={}"_fmt, item);
    if (shared_.discard_symbol(item.symbol)) {
      log::trace_1(R"(Drop symbol="{}")"_fmt, item.symbol);
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
        .currency = item.quote_asset,
        .settlement_currency = item.base_asset,
        .commission_currency = {},
        .tick_size = tick_size,
        .multiplier = NaN,
        .min_trade_vol = min_trade_vol,
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
  log::info("Exchange info: including symbols {}/{}"_fmt, counter, exchange_info.symbols.size());
  if (!symbols.empty()) {
    SymbolsUpdate symbols_update{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
}

}  // namespace huobi
}  // namespace roq
