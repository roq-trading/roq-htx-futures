/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/huobi_futures/config.hpp"

#include "roq/huobi_futures/tools/hasher.hpp"

namespace roq {
namespace huobi_futures {

class Security final {
 public:
  Security(Config const &, std::string_view const &account);

  Security(Security &&) = delete;
  Security(Security const &) = delete;

  std::string_view get_account() const { return account_; }
  std::string_view get_api_key() const { return key_; }

  std::pair<std::string, std::string> create_signature(std::chrono::nanoseconds now);

 private:
  const std::string account_;
  const std::string key_;
  tools::Hasher hasher_;
};

}  // namespace huobi_futures
}  // namespace roq
