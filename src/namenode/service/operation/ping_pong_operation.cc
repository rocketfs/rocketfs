// Copyright 2025 RocketFS

#include "namenode/service/operation/ping_pong_operation.h"

#include <coroutine>

#include <unifex/coroutine.hpp>

namespace rocketfs {

PingPongOperation::PingPongOperation(NameNodeContext* namenode_context,
                                     const PingPongRPC::Request& request)
    : namenode_context_(namenode_context), request_(request) {
}

unifex::task<PingPongRPC::Response> PingPongOperation::Run() {
  co_return PingPongRPC::Response{};
}

}  // namespace rocketfs
