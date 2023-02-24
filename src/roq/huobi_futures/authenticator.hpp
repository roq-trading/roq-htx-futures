/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/huobi_futures/config.hpp"

#include "roq/huobi_futures/tools/crypto.hpp"

namespace roq {
namespace huobi_futures {

struct Authenticator final {
  Authenticator(Config const &, std::string_view const &account);

  Authenticator(Authenticator &&) = delete;
  Authenticator(Authenticator const &) = delete;

  std::string_view get_account() const { return account_; }
  std::string_view get_api_key() const { return key_; }

  std::pair<std::string, std::string> create_signature(std::chrono::nanoseconds now);

 private:
  std::string const account_;
  std::string const key_;
  tools::Crypto crypto_;
};

}  // namespace huobi_futures
}  // namespace roq
