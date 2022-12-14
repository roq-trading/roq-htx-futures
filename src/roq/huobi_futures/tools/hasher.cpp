/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/huobi_futures/tools/hasher.hpp"

#include <fmt/format.h>

#include <array>
#include <cassert>

#include "roq/core/codec/hex.hpp"

using namespace std::literals;

namespace roq {
namespace huobi_futures {
namespace tools {

// === IMPLEMENTATION ===

Hasher::Hasher(std::string_view const &secret) : mac_{secret} {
}

std::pair<std::string, std::string> Hasher::create_signature(std::chrono::nanoseconds now) {
  auto timestamp = fmt::format("timestamp={}"sv, std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
  mac_.clear();
  mac_.update(timestamp);
  auto digest = mac_.final(digest_);
  std::string signature;
  core::codec::Hex::encode(signature, digest);
  return std::make_pair(timestamp, signature);
}

}  // namespace tools
}  // namespace huobi_futures
}  // namespace roq
