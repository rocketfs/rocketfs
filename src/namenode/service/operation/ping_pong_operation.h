// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_context.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using PingPongRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestPingPong>;

class PingPongOperation {
 public:
  PingPongOperation(NameNodeContext* namenode_context,
                    const PingPongRPC::Request& request);

  unifex::task<PingPongRPC::Response> Run();

 private:
  NameNodeContext* namenode_context_;
  const PingPongRPC::Request& request_;
};

}  // namespace rocketfs
