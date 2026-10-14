/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <cassert>
#include <chrono>

#include "roq/trace_info.hpp"

namespace roq {
namespace htx_futures {
namespace gateway {

struct Request final {
  bool ready() const { return open_orders.ready(); }

  struct {
    bool ready() const { return !downloading_; }

    // consumer
    void request(TraceInfo const &trace_info) { request_time_ = trace_info.origin_create_time; }

    bool has_response() const { return request_time_ < response_time_; }

    // producer
    template <typename Callback>
    void check_pending(TraceInfo const &trace_info, Callback callback) {
      assert(!downloading_);
      if (response_time_ < request_time_) {
        callback(trace_info);
        downloading_ = true;
      }
    }

    void commit(TraceInfo const &trace_info) {
      assert(downloading_);
      response_time_ = trace_info.origin_create_time;
      downloading_ = false;
    }

   private:
    std::chrono::nanoseconds request_time_ = {};
    std::chrono::nanoseconds response_time_ = {};
    bool downloading_ = {};
  } open_orders;
};

}  // namespace gateway
}  // namespace htx_futures
}  // namespace roq
