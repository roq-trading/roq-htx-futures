/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/htx_futures/json/parser.hpp"

namespace roq {
namespace htx_futures {

template <typename T>
struct ParserTester final : public json::Parser::Handler {
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
    ParserTester handler{callback};
    auto res = json::Parser::dispatch(handler, message, buffers, {}, false);
    CHECK(res == true);
    CHECK(handler.found_ == true);
  }

 protected:
  explicit ParserTester(callback_type const &callback) : callback_{callback} {}

  void operator()(Trace<json::Ping> const &event) override { dispatch(event); }
  void operator()(Trace<json::Error> const &event) override { dispatch(event); }
  void operator()(Trace<json::Subbed> const &event) override { dispatch(event); }
  void operator()(Trace<json::BBO> const &event) override { dispatch(event); }
  void operator()(Trace<json::Depth> const &event) override { dispatch(event); }
  void operator()(Trace<json::Trade> const &event) override { dispatch(event); }
  void operator()(Trace<json::Detail> const &event) override { dispatch(event); }
  void operator()(Trace<json::EstimatedRate> const &event) override { dispatch(event); }
  void operator()(Trace<json::PremiumIndex> const &event) override { dispatch(event); }
  void operator()(Trace<json::Basis> const &event) override { dispatch(event); }
  void operator()(Trace<json::Index> const &event) override { dispatch(event); }

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
