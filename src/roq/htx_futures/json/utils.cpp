/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/htx_futures/json/utils.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace json {

// === IMPLEMENTATION ===

Error guess_error(int32_t err_code) {
  return {};
}

Error guess_error(std::string_view const &message) {
  return {};
}

}  // namespace json
}  // namespace htx_futures
}  // namespace roq
