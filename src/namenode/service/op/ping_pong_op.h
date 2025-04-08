// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_ctx.h"
#include "namenode/service/op/op_base.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using PingPongRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestPingPong>;

class PingPongOp : public OpBase<PingPongRPC> {
 public:
  PingPongOp(NameNodeCtx* namenode_ctx, const PingPongRPC::Request& req);

  unifex::task<PingPongRPC::Response> Run();

 private:
  NameNodeCtx* namenode_ctx_;
  const PingPongRPC::Request& req_;
};

}  // namespace rocketfs
