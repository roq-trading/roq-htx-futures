/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/huobi_futures/authenticator.hpp"

namespace roq {
namespace huobi_futures {

// === IMPLEMENTATION ===

Authenticator::Authenticator(Config const &config, std::string_view const &account)
    : account_{account}, key_{config.get_api_key(account_)}, crypto_{config.get_secret(account_)} {
}

std::pair<std::string, std::string> Authenticator::create_signature(std::chrono::nanoseconds now) {
  return crypto_.create_signature(now);
}

}  // namespace huobi_futures
}  // namespace roq
