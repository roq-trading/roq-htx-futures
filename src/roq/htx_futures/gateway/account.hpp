/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/margin_mode.hpp"

#include "roq/htx_futures/gateway/config.hpp"

#include "roq/htx_futures/tools/crypto.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct Account final {
  Account(Config const &, std::string_view const &name, MarginMode, roq::io::web::URI const &uri);

  Account(Account const &) = delete;

  std::string_view get_api_key() const { return crypto_.key; }

  std::string_view create_ws_auth(std::string_view const &path, std::chrono::seconds now_utc);

  std::string_view create_query(web::http::Method, std::string_view const &path, std::chrono::seconds now_utc);

  std::string const name;
  MarginMode const margin_mode;

 private:
  tools::Crypto crypto_;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
