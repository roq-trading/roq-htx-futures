/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/drop_copy.hpp"

#include "roq/mask.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/huobi_futures/flags.hpp"

#include "roq/huobi_futures/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

namespace {
const auto NAME = "dc"sv;
const auto SUPPORTS = Mask{
    SupportType::FUNDS,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::ws_order_uri();
  core::web::ClientSocket::Config config{
      .validate_certificate = server::Flags::tls_validate_certificate(),
      .uris = {&uri, 1},
      .query = {},
      .ping_frequency = Flags::ws_ping_freq(),
      .read_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
  };
  return core::web::ClientSocket{handler, context, config, []() { return std::string(); }};
}
}  // namespace

DropCopy::DropCopy(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      connection_(create_connection(*this, context)), decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
          .ping = create_metrics(name_, "ping"sv),
          .close = create_metrics(name_, "close"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
      },
      security_(security), shared_(shared), inflate_(core::zlib::Inflate::GZIP_NO_HEADER) {
}

void DropCopy::operator()(const Event<Start> &) {
  connection_.start();
}

void DropCopy::operator()(const Event<Stop> &) {
  connection_.stop();
}

void DropCopy::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void DropCopy::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

void DropCopy::operator()(const core::web::ClientSocket::Connected &) {
}

void DropCopy::operator()(const core::web::ClientSocket::Disconnected &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
}

void DropCopy::operator()(const core::web::ClientSocket::Ready &) {
  (*this)(ConnectionStatus::READY);
}

void DropCopy::operator()(const core::web::ClientSocket::Close &) {
}

void DropCopy::operator()(const core::web::ClientSocket::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = security_.get_account(),
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(const core::web::ClientSocket::Text &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(const core::web::ClientSocket::Binary &binary) {
  if (inflate_.decode(binary.payload, inflate_buffer_, [&](auto &payload) {
        std::string_view message{
            reinterpret_cast<char const *>(std::data(payload)), std::size(payload)};
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
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
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
  connection_.send_text(message);
}

void DropCopy::parse(const std::string_view &message) {
  profile_.parse([&]() {
    try {
      auto trace_info = server::create_trace_info();
      core::json::Buffer buffer(decode_buffer_);
      json::Parser2::dispatch(*this, message, buffer, trace_info);
    } catch (...) {
      log::warn(R"(message="{}")"sv, message);
      core::tools::UnhandledException::terminate();
    }
  });
}

void DropCopy::operator()(const Trace<json::Ping> &event) {
  profile_.ping([&]() {
    auto &[trace_info, ping] = event;
    send_pong(ping.timestamp);
  });
}

void DropCopy::operator()(const Trace<json::Close> &event) {
  profile_.close([&]() {
    auto &[trace_info, close] = event;
    log::warn("trace_info={}, close={}"sv, trace_info, close);
    connection_.close();
  });
}

void DropCopy::operator()(const Trace<json::FundingRate> &) {
  log::fatal("Unexpected"sv);
}

}  // namespace huobi_futures
}  // namespace roq
