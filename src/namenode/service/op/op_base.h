// Copyright 2025 RocketFS

#pragma once

#include <string>

#include "common/status.h"
#include "namenode/namenode_ctx.h"
#include "namenode/service/handler_ctx.h"

namespace rocketfs {

template <typename RPC>
class OpBase {
 public:
  explicit OpBase(NameNodeCtx* namenode_ctx);
  OpBase(const OpBase&) = delete;
  OpBase(OpBase&&) = delete;
  OpBase& operator=(const OpBase&) = delete;
  OpBase& operator=(OpBase&&) = delete;
  ~OpBase() = default;

 protected:
  HandlerCtx handler_ctx_;
};

template <typename RPC>
OpBase<RPC>::OpBase(NameNodeCtx* namenode_ctx)
    : handler_ctx_(CHECK_NOTNULL(namenode_ctx)) {
}

}  // namespace rocketfs
