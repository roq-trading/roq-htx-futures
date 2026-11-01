/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/utils/container.hpp"

#include "roq/core/limit/request.hpp"

#include "roq/htx_futures/gateway/shared.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct Request final : public core::limit::Request::Handler {
  explicit Request(Shared &);

  // core::Request::Handler

  bool ready() const override;
  bool can_request(TraceInfo const &) override;

  // ...

  core::limit::Request open_orders;
  core::limit::Request lost_orders;

  utils::unordered_set<std::pair<uint8_t, uint64_t>> details_lost_orders;

 private:
  Shared &shared_;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
