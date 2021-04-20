/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/huobi/security.h"

#include <cassert>

#include "roq/core/binascii/hex.h"

#include "roq/core/crypto/hmac.h"

namespace roq {
namespace huobi {

Security::Security(const Config &config, const std::string_view &account)
    : account_(account), key_(config.get_api_key(account_)), hmac_(config.get_secret(account_)) {
}

std::pair<std::string, std::string> Security::create_signature(
    const std::chrono::nanoseconds &now) {
  auto timestamp = roq::format(
      "timestamp={}"_fmt, std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
  hmac_.clear();
  hmac_.update(timestamp);
  std::array<char, 32u> buffer;
  auto length = hmac_.digest(buffer);
  assert(length == buffer.size());
  auto signature = core::binascii::Hex::encode(buffer);
  return std::make_pair(timestamp, signature);
}

}  // namespace huobi
}  // namespace roq
