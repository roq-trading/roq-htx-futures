/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

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

  core::limit::Request orders;

 private:
  Shared &shared_;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
