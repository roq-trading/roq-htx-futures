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

Error guess_error_v5(int32_t code) {
  switch (code) {
    case 200:
      break;
    case 1071:  // Repeated cancellation. Your order has been canceled.
      return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
    case 1041:  // The amount has exceeded the limit (580000 Cont), please modify and place order again.
      return Error::INSUFFICIENT_FUNDS;
  }
  return {};
}

}  // namespace json
}  // namespace protocol
}  // namespace htx_futures
}  // namespace roq
