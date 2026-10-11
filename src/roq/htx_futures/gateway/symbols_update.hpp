/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <span>

#include "roq/string_types.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct SymbolsUpdate final {
  std::span<Symbol const> symbols;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
