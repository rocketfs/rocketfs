#pragma once

namespace rocketfs {

class HandlerContext {
 public:
  HandlerContext(const HandlerContext&) = delete;
  HandlerContext(HandlerContext&&) = delete;
  HandlerContext& operator=(const HandlerContext&) = delete;
  HandlerContext& operator=(HandlerContext&&) = delete;
  ~HandlerContext() = default;
};

}  // namespace rocketfs
