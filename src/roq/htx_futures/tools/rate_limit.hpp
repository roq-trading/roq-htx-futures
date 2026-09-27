/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>

#include <fmt/format.h>

#include "roq/web/rest/interceptor.hpp"

#include "roq/web/socket/interceptor.hpp"

#include "roq/htx_futures/flags/settings.hpp"

namespace roq {
namespace htx_futures {
namespace tools {

struct RateLimit final : public web::rest::Interceptor, public web::socket::Interceptor {
  explicit RateLimit(flags::Settings const &);

  struct Params {
    int32_t interval = {};
    int32_t limit = {};
    int32_t remaining = {};
    int64_t reset = {};  // msec
  };

 protected:
  // web::Interceptor

  operator std::chrono::nanoseconds() const override { return suspend_until_; }

  // web::rest::Interceptor

  void operator()(Trace<web::rest::MessageBegin> const &) override;
  void operator()(Trace<web::rest::MessageHeader> const &) override;
  void operator()(Trace<web::rest::MessageEnd> const &) override;

  // web::socket::Interceptor

 private:
  bool const suspend_on_rate_limit_;

  Params params_;

  std::chrono::nanoseconds suspend_until_ = {};
};

}  // namespace tools
}  // namespace htx_futures
}  // namespace roq

template <>
struct fmt::formatter<roq::htx_futures::tools::RateLimit::Params> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::htx_futures::tools::RateLimit::Params const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(interval={}, )"
        R"(limit={}, )"
        R"(remaining={}, )"
        R"(reset={})"
        R"(}})"sv,
        value.interval,
        value.limit,
        value.remaining,
        value.reset);
  }
};
