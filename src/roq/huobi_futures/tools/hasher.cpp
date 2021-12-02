/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/huobi_futures/tools/hasher.h"

#include <fmt/format.h>

#include <array>
#include <cassert>

#include "roq/core/binascii/hex.h"

#include "roq/core/crypto/hmac.h"

using namespace std::literals;

namespace roq {
namespace huobi_futures {
namespace tools {

Hasher::Hasher(const std::string_view &secret) : hmac_(secret) {
}

std::pair<std::string, std::string> Hasher::create_signature(std::chrono::nanoseconds now) {
  auto timestamp = fmt::format(
      "timestamp={}"sv, std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
  hmac_.clear();
  hmac_.update(timestamp);
  std::array<char, 32> buffer;
  auto length = hmac_.digest(buffer);
  assert(length == std::size(buffer));
  auto signature = core::binascii::Hex::encode(buffer);
  return std::make_pair(timestamp, signature);
}

}  // namespace tools
}  // namespace huobi_futures
}  // namespace roq
