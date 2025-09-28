/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/htx_futures/config.hpp"

#include "roq/htx_futures/tools/crypto.hpp"

namespace roq {
namespace htx_futures {

struct Account final {
  Account(Config const &, std::string_view const &name);

  Account(Account const &) = delete;

  std::string_view get_name() const { return name_; }
  std::string_view get_api_key() const { return key_; }

  std::pair<std::string, std::string> create_signature(std::chrono::nanoseconds now);

 private:
  std::string const name_;
  std::string const key_;
  tools::Crypto crypto_;
};

}  // namespace htx_futures
}  // namespace roq
