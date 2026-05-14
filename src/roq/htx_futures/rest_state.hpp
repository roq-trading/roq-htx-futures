/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <cstdint>

namespace roq {
namespace htx_futures {

enum class RestState : uint8_t {
  UNDEFINED = 0,
  CONTRACT_INFO,
  DONE,
};

}  // namespace htx_futures
}  // namespace roq
