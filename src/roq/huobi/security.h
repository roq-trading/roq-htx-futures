/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/format.h"

#include "roq/core/crypto/hmac.h"

#include "roq/core/http/method.h"

#include "roq/huobi/config.h"

namespace roq {
namespace huobi {

class Security final {
 public:
  Security(const Config &, const std::string_view &account);

  Security(Security &&) = delete;
  Security(const Security &) = delete;

  std::string_view get_account() const { return account_; }

  std::string_view get_api_key() const { return key_; }

  std::pair<std::string, std::string> create_signature(const std::chrono::nanoseconds &now);

 private:
  const std::string account_;
  const std::string key_;
  core::crypto::HMAC_SHA256 hmac_;
};

}  // namespace huobi
}  // namespace roq
