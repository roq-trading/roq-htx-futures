/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/htx_futures/protocol/json/utils.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace htx_futures {
namespace protocol {
namespace json {

// === IMPLEMENTATION ===

Error guess_error([[maybe_unused]] int32_t err_code) {
  return {};
}

Error guess_error([[maybe_unused]] std::string_view const &message) {
  return {};
}

}  // namespace json
}  // namespace protocol
}  // namespace htx_futures
}  // namespace roq
