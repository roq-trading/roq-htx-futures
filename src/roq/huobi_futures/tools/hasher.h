/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/core/crypto/hmac.h"

namespace roq {
namespace huobi_futures {
namespace tools {

class Hasher final {
 public:
  explicit Hasher(const std::string_view &secret);

  Hasher(Hasher &&) = delete;
  Hasher(const Hasher &) = delete;

  std::pair<std::string, std::string> create_signature(std::chrono::nanoseconds now);

 private:
  core::crypto::HMAC_SHA256 hmac_;
};

}  // namespace tools
}  // namespace huobi_futures
}  // namespace roq
