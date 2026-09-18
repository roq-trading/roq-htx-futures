/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/server.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct DropCopy {
  struct Handler {};

  DropCopy() = default;

  DropCopy(DropCopy const &) = delete;

  virtual ~DropCopy() = default;

  virtual void operator()(Event<Start> const &) = 0;
  virtual void operator()(Event<Stop> const &) = 0;
  virtual void operator()(Event<Timer> const &) = 0;

  virtual void operator()(metrics::Writer &) const = 0;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
