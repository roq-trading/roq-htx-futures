/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/logging.h"
#include "roq/server.h"

namespace roq {
namespace huobi_futures {

class Config final : public server::Config, public server::ConfigReader::Handler {
 public:
  explicit Config(const std::string_view &path);

  std::string get_master_account() const;

  std::string get_api_key(const std::string_view &account) const;
  std::string get_secret(const std::string_view &account) const;

 protected:
  // server::Config
  void dispatch(server::Config::Handler &) const override;

  // server::ConfigReader::Handler
  void operator()(server::Symbols &&) override;
  void operator()(server::Account &&) override;
  void operator()(server::User &&) override;
  void operator()(const std::string_view &key, toml::node &) override;

 public:
  std::vector<server::User> users;
  server::Symbols symbols;
  absl::flat_hash_map<std::string, server::Account> accounts;
  std::string master_account_;
};

/*
 * REST API
 * https://api-public.sandbox.pro.binance.com
 *
 * Websocket Feed
 * wss://ws-feed-public.sandbox.pro.binance.com
 *
 * FIX API
 * tcp+ssl://fix-public.sandbox.pro.binance.com:4198
 */

}  // namespace huobi_futures
}  // namespace roq

template <>
struct fmt::formatter<roq::huobi_futures::Config> : public roq::formatter {
  template <typename C>
  auto format(const roq::huobi_futures::Config &value, C &ctx) {
    using namespace roq::literals;
    return roq::format_to(
        ctx.out(),
        R"({{)"
        R"(symbols={}, )"
        R"(users=[{}], )"
        R"(accounts=[{}], )"
        R"(master_account="{}")"
        R"(}})"_sv,
        value.symbols,
        roq::join(value.users, ", "_sv),
        roq::join(value.accounts, ", "_sv),
        value.master_account_);
  }
};
