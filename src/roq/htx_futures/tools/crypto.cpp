/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/tools/crypto.hpp"

#include <fmt/format.h>

#include <cassert>
#include <iterator>

#include "roq/logging.hpp"

#include "roq/utils/codec/base64.hpp"
#include "roq/utils/codec/url.hpp"

#include <roq/utils/mac/hmac.hpp>

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace tools {

// === HELPERS ===

namespace {
auto create_hmac_sha256(auto const &secret) {
  return utils::mac::HMAC<utils::hash::SHA256>{secret};
}

template <typename R>
auto create_ed25519(auto &secret) {
  using result_type = std::remove_cvref_t<R>;
  return result_type::create(secret, false);
}
}  // namespace

// === IMPLEMENTATION ===

Crypto::Crypto(std::string_view const &key, std::string_view const &secret, std::string_view const &hostname)
    : key{key}, hostname_{hostname},
#ifdef USE_ED25
      pkey_{create_ed25519<decltype(pkey_)>(secret)}
#else
      mac_{secret}
#endif
{
}

std::string_view Crypto::create_query(web::http::Method method, std::string_view const &path, std::chrono::seconds now_utc) {
  assert(!std::empty(path));
  encode_buffer_.clear();
  std::chrono::sys_days days{std::chrono::duration_cast<std::chrono::days>(now_utc)};
  std::chrono::year_month_day ymd{days};
  auto tmp = std::chrono::time_point_cast<std::chrono::seconds>(days);
  auto tmp2 = std::chrono::duration_cast<std::chrono::seconds>(tmp.time_since_epoch());
  auto tmp3 = now_utc - tmp2;
  std::chrono::hh_mm_ss hms{tmp3};
#ifdef USE_ED25
  fmt::format_to(
      std::back_inserter(encode_buffer_),
      "?AccessKeyId={}&"
      "SignatureMethod=Ed25519&"
      "SignatureVersion=2&"
      "Timestamp={:04}-{:02}-{:02}T{:02}%3A{:02}%3A{:02}"sv,
      key,
      static_cast<int>(ymd.year()),
      static_cast<unsigned>(ymd.month()),
      static_cast<unsigned>(ymd.day()),
      hms.hours().count(),
      hms.minutes().count(),
      hms.seconds().count());
  auto tmp4 = std::string_view{encode_buffer_}.substr(1);
  digest_.clear();
  context_.reset();
  auto payload = fmt::format("{}\n{}\n{}\n{}"sv, magic_enum::enum_name(method), hostname_, path, tmp4);
  pkey_.sign(digest_, payload, context_);
  std::string signature;
  utils::codec::Base64::encode(signature, digest_, true, false);
  fmt::format_to(std::back_inserter(encode_buffer_), "&Signature={}"sv, signature);
#else
  fmt::format_to(
      std::back_inserter(encode_buffer_),
      "?AccessKeyId={}&"
      "SignatureMethod=HmacSHA256&"
      "SignatureVersion=2&"
      "Timestamp={:04}-{:02}-{:02}T{:02}%3A{:02}%3A{:02}"sv,
      key,
      static_cast<int>(ymd.year()),
      static_cast<unsigned>(ymd.month()),
      static_cast<unsigned>(ymd.day()),
      hms.hours().count(),
      hms.minutes().count(),
      hms.seconds().count());
  auto tmp4 = std::string_view{encode_buffer_}.substr(1);
  mac_.clear();
  mac_.update(magic_enum::enum_name(method));
  mac_.update("\n"sv);
  mac_.update(hostname_);
  mac_.update("\n"sv);
  mac_.update(path);
  mac_.update("\n"sv);
  mac_.update(tmp4);
  auto digest = mac_.final(digest_);
  std::string signature;
  utils::codec::Base64::encode(signature, digest, false, false);
  std::string buffer_2;
  auto signature_2 = utils::codec::URL::encode(buffer_2, signature);
  fmt::format_to(std::back_inserter(encode_buffer_), "&Signature={}"sv, signature_2);
#endif
  return encode_buffer_;
}

}  // namespace tools
}  // namespace htx_futures
}  // namespace roq
