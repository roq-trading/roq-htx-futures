/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

#include <roq/utils/hash/sha256.hpp>

#include "roq/web/http/method.hpp"

#include "roq/utils/mac/hmac.hpp"

#include "roq/utils/signature/context.hpp"
#include "roq/utils/signature/pkey.hpp"

// #define USE_ED25

namespace roq {
namespace htx_futures {
namespace tools {

struct Crypto final {
  Crypto(std::string_view const &key, std::string_view const &secret, std::string_view const &hostname);

  Crypto(Crypto &&) = delete;
  Crypto(Crypto const &) = delete;

  std::string_view create_ws_auth(std::string_view const &path, std::chrono::seconds now_utc);

  std::string_view create_query(web::http::Method, std::string_view const &path, std::chrono::seconds now_utc);

  std::string const key;

 private:
  std::string const hostname_;
  std::string encode_buffer_;
#ifdef USE_ED25
  utils::signature::PKey pkey_;
  utils::signature::Context context_;
  std::vector<std::byte> digest_;
#else
  using MAC = utils::mac::HMAC<utils::hash::SHA256>;
  using Digest = std::array<std::byte, MAC::DIGEST_LENGTH>;
  MAC mac_;
  Digest digest_;
#endif
};

}  // namespace tools
}  // namespace htx_futures
}  // namespace roq
