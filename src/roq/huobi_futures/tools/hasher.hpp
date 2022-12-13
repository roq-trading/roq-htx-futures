/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <array>
#include <chrono>
#include <string>
#include <string_view>

#include "roq/core/mac/hmac.hpp"

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
  using MAC = core::mac::HMAC<core::hash::SHA256>;
  using Digest = std::array<std::byte, MAC::DIGEST_LENGTH>;

  MAC mac_;
  Digest digest_;
};

}  // namespace tools
}  // namespace huobi_futures
}  // namespace roq
