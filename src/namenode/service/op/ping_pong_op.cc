// Copyright 2025 RocketFS

#include "namenode/service/op/ping_pong_op.h"

#include <coroutine>

#include <unifex/coroutine.hpp>

#include "common/logger.h"

namespace rocketfs {

PingPongOp::PingPongOp(NameNodeCtx* namenode_ctx,
                       const PingPongRPC::Request& req)
    : OpBase(CHECK_NOTNULL(namenode_ctx)), req_(req) {
}

unifex::task<PingPongRPC::Response> PingPongOp::Run() {
  co_return PingPongRPC::Response{};
}

}  // namespace rocketfs
