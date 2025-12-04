/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/parser_3.hpp"

namespace roq {
namespace htx_futures {

template <typename T>
struct Parser3Tester final : public json::Parser3::Handler {
  using value_type = std::remove_cvref_t<T>;
  using callback_type = std::function<void(value_type const &)>;

  static void dispatch(callback_type const &callback, std::string_view const &message, size_t buffer_size, size_t max_depth) {
    core::json::BufferStack buffers{buffer_size, max_depth};
    // simple
    // XXX FIXME TODO catch2 block ???
    T obj{message, buffers};
    callback(obj);
    // parser
    // XXX FIXME TODO catch2 block ???
    Parser3Tester handler{callback};
    auto res = json::Parser3::dispatch(handler, message, buffers, {}, false);
    CHECK(res == true);
    CHECK(handler.found_ == true);
  }

 protected:
  explicit Parser3Tester(callback_type const &callback) : callback_{callback} {}

  void operator()(Trace<json::Close2> const &event) override { dispatch(event); }
  void operator()(Trace<json::Error2> const &event) override { dispatch(event); }
  void operator()(Trace<json::Ping> const &event) override { dispatch(event); }
  void operator()(Trace<json::Auth> const &event) override { dispatch(event); }
  void operator()(Trace<json::Response> const &event) override { dispatch(event); }

  template <typename U>
  void dispatch(Trace<U> const &event) {
    if constexpr (std::is_invocable_v<callback_type, U>) {
      found_ = true;
      callback_(event);
    } else {
      FAIL();
    }
  }

 private:
  callback_type const callback_;
  bool found_ = false;
};

}  // namespace htx_futures
}  // namespace roq
