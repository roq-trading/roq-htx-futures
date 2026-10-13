/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/gateway/account.hpp"

#include "roq/utils/safe_cast.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace gateway {

// === HELPERS ===

namespace {
template <typename R>
auto create_crypto(auto &config, auto &name, auto &uri) {
  using result_type = std::remove_cvref_t<R>;
  log::warn(R"(DEBUG uri={})"sv, uri);
  return result_type{config.get_api_key(name), config.get_secret(name), uri.get_host()};
}
}  // namespace

// === IMPLEMENTATION ===

Account::Account(Config const &config, std::string_view const &name, MarginMode margin_mode, roq::io::web::URI const &uri)
    : name{name}, margin_mode{margin_mode}, crypto_{create_crypto<decltype(crypto_)>(config, name, uri)} {
}

std::string_view Account::create_ws_auth(TraceInfo const &trace_info, std::string_view const &path) {
  return crypto_.create_ws_auth(path, utils::safe_cast(trace_info.origin_create_time_utc));
}

std::string_view Account::create_query(TraceInfo const &trace_info, web::http::Method method, std::string_view const &path) {
  return crypto_.create_query(method, path, utils::safe_cast(trace_info.origin_create_time_utc));
}

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
