/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi_futures/config.h"

#include <utility>

#include "roq/logging.h"

#include "roq/huobi_futures/flags.h"

using namespace std::literals;

namespace roq {
namespace huobi_futures {

Config::Config(const std::string_view &config_path, const std::string_view &secrets_path) {
  server::ConfigReader::parse_file(*this, config_path, secrets_path);
  assert(!master_account_.empty());
}

std::string Config::get_master_account() const {
  return master_account_;
}

std::string Config::get_api_key(const std::string_view &account) const {
  auto iter = accounts.find(account);
  if (iter == accounts.end()) {
    log::fatal(R"(Unknown account="{}")"sv, account);
  }
  return (*iter).second.login;
}

std::string Config::get_secret(const std::string_view &account) const {
  auto iter = accounts.find(account);
  if (iter == accounts.end()) {
    log::fatal(R"(Unknown account="{}")"sv, account);
  }
  return (*iter).second.secret;
}

void Config::dispatch(server::Config::Handler &handler) const {
  handler(Flags::exchange());
  handler(symbols);
  for (auto &iter : accounts)
    handler(iter.second);
  for (auto &user : users)
    handler(user);
  server::Settings settings{
      .supports{
          SupportType::REFERENCE_DATA,
          SupportType::MARKET_STATUS,
          SupportType::TOP_OF_BOOK,
          SupportType::MARKET_BY_PRICE,
          SupportType::TRADE_SUMMARY,
          SupportType::STATISTICS,
          SupportType::CREATE_ORDER,
          SupportType::CANCEL_ORDER,
          SupportType::ORDER_ACK,
          SupportType::FUNDS,
      },
      .mbp_max_depth = Flags::ws_subscribe_depth_levels(),
      .mbp_tick_size_multiplier = NaN,
      .mbp_min_trade_vol_multiplier = NaN,
      .mbp_allow_remove_non_existing = {},
      .mbp_allow_price_inversion = Flags::mbp_allow_price_inversion(),
      .oms_request_id_type = server::RequestIdType::BASE64,
      .oms_download_has_state = {},
      .oms_download_has_routing_id = {},
  };
  handler(settings);
  for (auto &iter : rate_limits)
    handler(iter.second);
}

void Config::operator()(server::Symbols &&symbols) {
  (*this).symbols = std::move(symbols);
}

void Config::operator()(server::Account &&account) {
  if (account.master)
    master_account_ = account.name;
  accounts.emplace(account.name, std::move(account));
}

void Config::operator()(server::User &&user) {
  users.emplace_back(std::move(user));
}

void Config::operator()(server::RateLimit &&rate_limit) {
  rate_limits.emplace(rate_limit.name, std::move(rate_limit));
}

void Config::operator()(const std::string_view &key, toml::node &) {
  log::warn(R"(Unexpected: key="{}")"sv, key);
}

}  // namespace huobi_futures
}  // namespace roq
