/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi_futures/config.h"

#include <utility>

#include "roq/logging.h"

#include "roq/huobi_futures/flags.h"

using namespace roq::literals;

namespace roq {
namespace huobi_futures {

Config::Config(const std::string_view &path) {
  server::ConfigReader::parse(*this, path);
}

std::string Config::get_master_account() const {
  return master_account_;
}

std::string Config::get_api_key(const std::string_view &account) const {
  auto iter = accounts.find(account);
  if (iter == accounts.end()) {
    log::fatal(R"(Unknown account="{}")"_sv, account);
  }
  return (*iter).second.login;
}

std::string Config::get_secret(const std::string_view &account) const {
  auto iter = accounts.find(account);
  if (iter == accounts.end()) {
    log::fatal(R"(Unknown account="{}")"_sv, account);
  }
  return (*iter).second.secret;
}

void Config::dispatch(server::Config::Handler &handler) const {
  handler(Flags::exchange());
  handler(symbols);
  for (auto iter : accounts)
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
      .mbp_allow_price_inversion = {},
      .mbp_allow_fractional_tick_size = {},
      .mbp_allow_remove_non_existing = {},
      .oms_download_has_state = {},
      .oms_download_has_routing_id = {},
  };
  handler(settings);
}

void Config::operator()(server::Symbols &&symbols) {
  (*this).symbols = std::move(symbols);
}

void Config::operator()(server::Account &&account) {
  auto res = accounts.emplace(account.name, std::move(account));
  if (master_account_.empty())
    master_account_ = (*res.first).first;
}

void Config::operator()(server::User &&user) {
  users.emplace_back(std::move(user));
}

void Config::operator()(const std::string_view &key, cpptoml::base &) {
  log::warn(R"(UNKNOWN KEY="{}")"_sv, key);
}

}  // namespace huobi_futures
}  // namespace roq
