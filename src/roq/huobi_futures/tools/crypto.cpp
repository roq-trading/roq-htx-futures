/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/huobi_futures/tools/crypto.hpp"

#include <fmt/format.h>

#include "roq/utils/codec/hex.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {
namespace tools {

// === IMPLEMENTATION ===

Crypto::Crypto(std::string_view const &secret) : mac_{secret} {
}

std::pair<std::string, std::string> Crypto::create_signature(std::chrono::nanoseconds now) {
  auto timestamp = fmt::format("timestamp={}"sv, std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
  mac_.clear();
  mac_.update(timestamp);
  auto digest = mac_.final(digest_);
  std::string signature;
  utils::codec::Hex::encode(signature, digest);
  return {timestamp, signature};
}

}  // namespace tools
}  // namespace huobi_futures
}  // namespace roq
