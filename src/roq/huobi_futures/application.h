/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include "roq/service.h"

namespace roq {
namespace huobi_futures {

class Application final : public roq::Service {
 public:
  using roq::Service::Service;

 protected:
  int main(int, char **) override;
};

}  // namespace huobi_futures
}  // namespace roq
