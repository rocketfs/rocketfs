// Copyright 2025 RocketFS

#include "namenode/service/operation/ping_pong_op.h"

#include <coroutine>

#include <unifex/coroutine.hpp>

namespace rocketfs {

PingPongOp::PingPongOp(NameNodeCtx* namenode_ctx,
                       const PingPongRPC::Request& req)
    : namenode_ctx_(namenode_ctx), req_(req) {
}

unifex::task<PingPongRPC::Response> PingPongOp::Run() {
  co_return PingPongRPC::Response{};
}

}  // namespace rocketfs
