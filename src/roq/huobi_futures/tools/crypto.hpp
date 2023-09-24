/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <roq/utils/hash/sha256.hpp>

#include "roq/utils/mac/hmac.hpp"

namespace roq {
namespace huobi_futures {
namespace tools {

struct Crypto final {
  explicit Crypto(std::string_view const &secret);

  Crypto(Crypto &&) = delete;
  Crypto(Crypto const &) = delete;

  std::pair<std::string, std::string> create_signature(std::chrono::nanoseconds now);

 private:
  using MAC = utils::mac::HMAC<utils::hash::SHA256>;
  using Digest = std::array<std::byte, MAC::DIGEST_LENGTH>;

  MAC mac_;
  Digest digest_;
};

}  // namespace tools
}  // namespace huobi_futures
}  // namespace roq
