/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/tools/rate_limit.hpp"

#include "roq/utils/compare.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/hash/fnv.hpp"

#include "roq/utils/charconv/from_chars.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace tools {

// === CONSTANTS ===

namespace {
auto const DEFAULT_BACKOFF = 5s;     // note! maybe as low as 1 second
auto const BLOCKED_BACKOFF = 10min;  // note! very serious
}  // namespace

// === HELPERS ===

namespace {
// note! std::tolower is not constexpr gcc16 + clang23
constexpr auto lower(auto value) {
  return utils::detail::ascii_to_lower(value);
}

enum class Header {
  UNKNOWN,
  RATELIMIT_INTERVAL,
  RATELIMIT_LIMIT,
  RATELIMIT_REMAINING,
  RATELIMIT_RESET,
};

constexpr auto parse_header(std::string_view const &text) {
  std::string value;
  value.reserve(std::size(text));
  std::transform(std::begin(text), std::end(text), std::back_inserter(value), [](auto c) { return lower(c); });
  auto key = utils::hash::FNV::compute(value);
  switch (key) {
    case utils::hash::FNV::compute("ratelimit-interval"sv):
      return Header::RATELIMIT_INTERVAL;
    case utils::hash::FNV::compute("ratelimit-limit"sv):
      return Header::RATELIMIT_LIMIT;
    case utils::hash::FNV::compute("ratelimit-remaining"sv):
      return Header::RATELIMIT_REMAINING;
    case utils::hash::FNV::compute("ratelimit-reset"sv):
      return Header::RATELIMIT_RESET;
  }
  return Header::UNKNOWN;
}

static_assert(parse_header("ratelimit-interval"sv) == Header::RATELIMIT_INTERVAL);
static_assert(parse_header("ratelimit-limit"sv) == Header::RATELIMIT_LIMIT);
static_assert(parse_header("ratelimit-remaining"sv) == Header::RATELIMIT_REMAINING);
static_assert(parse_header("ratelimit-reset"sv) == Header::RATELIMIT_RESET);
}  // namespace

// === IMPLEMENTATION ===

RateLimit::RateLimit(flags::Settings const &settings) : suspend_on_rate_limit_{settings.experimental.suspend_on_rate_limit} {
}

// web::rest::Interceptor

void RateLimit::operator()(Trace<web::rest::MessageBegin> const &) {
}

void RateLimit::operator()(Trace<web::rest::MessageHeader> const &event) {
  auto &[trace_info, header] = event;
  auto update_value = [&](auto &result) {
    using value_type = std::remove_cvref_t<decltype(result)>;
    auto value = utils::charconv::from_chars<value_type>(header.value);
    return utils::update(result, value);
  };
  auto update_suspend_until = [&]() {
    if (!suspend_on_rate_limit_) {
      return;
    }
    if (params_.remaining > 0 || params_.reset == 0) {
      suspend_until_ = {};
    } else {
      auto now = clock::get_system();
      auto now_utc = clock::get_realtime();
      auto timestamp = std::chrono::milliseconds{params_.reset};
      if (now_utc < timestamp) {
        auto period = timestamp - now_utc;
        suspend_until_ = std::max(suspend_until_, now + period);
      } else {
        suspend_until_ = std::max(suspend_until_, now + DEFAULT_BACKOFF);
      }
    }
  };
  auto key = parse_header(header.name);
  switch (key) {
    using enum Header;
    [[likely]] case UNKNOWN:
      return;
    case RATELIMIT_INTERVAL:
      update_value(params_.interval);
      break;
    case RATELIMIT_LIMIT:
      update_value(params_.limit);
      break;
    case RATELIMIT_REMAINING:
      if (update_value(params_.remaining)) {
        update_suspend_until();
      }
      break;
    case RATELIMIT_RESET:
      if (update_value(params_.reset)) {
        update_suspend_until();
      }
      break;
  }
}

void RateLimit::operator()(Trace<web::rest::MessageEnd> const &event) {
  auto &[trace_info, message_end] = event;
  if (!suspend_on_rate_limit_) {
    return;
  }
  switch (message_end.status) {
    using enum web::http::Status;
    [[unlikely]] case FORBIDDEN: {  // 403
      auto now = clock::get_system();
      suspend_until_ = std::max(suspend_until_, now + BLOCKED_BACKOFF);
      break;
    }
    [[unlikely]] case TOO_MANY_REQUESTS: {  // 429
      if (suspend_until_.count() == 0) {
        auto now = clock::get_system();
        suspend_until_ = now + DEFAULT_BACKOFF;
      }
      break;
    }
    default:
      break;
  }
}

// web::socket::Interceptor

}  // namespace tools
}  // namespace htx_futures
}  // namespace roq
