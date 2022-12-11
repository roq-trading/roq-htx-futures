/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/core/crypto/hmac_sha256.hpp"

namespace roq {
namespace huobi_futures {
namespace tools {

class Hasher final {
 public:
  explicit Hasher(std::string_view const &secret);

  Hasher(Hasher &&) = delete;
  Hasher(Hasher const &) = delete;

  std::pair<std::string, std::string> create_signature(std::chrono::nanoseconds now);

 private:
  core::crypto::HMAC_SHA256 hmac_;
};

}  // namespace tools
}  // namespace huobi_futures
}  // namespace roq
