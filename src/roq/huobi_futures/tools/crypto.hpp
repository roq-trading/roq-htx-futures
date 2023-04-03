/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <roq/core/hash/sha256.hpp>

#include "roq/core/mac/hmac.hpp"

namespace roq {
namespace huobi_futures {
namespace tools {

struct Crypto final {
  explicit Crypto(std::string_view const &secret);

  Crypto(Crypto &&) = delete;
  Crypto(Crypto const &) = delete;

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
