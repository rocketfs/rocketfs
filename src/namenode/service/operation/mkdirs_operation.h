// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_context.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using MkdirsRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestMkdirs>;

class MkdirsOperation {
 public:
  MkdirsOperation(NameNodeContext* namenode_context,
                  const MkdirsRPC::Request& request);
  MkdirsOperation(const MkdirsOperation&) = delete;
  MkdirsOperation(MkdirsOperation&&) = delete;
  MkdirsOperation& operator=(const MkdirsOperation&) = delete;
  MkdirsOperation& operator=(MkdirsOperation&&) = delete;
  ~MkdirsOperation() = default;

  unifex::task<MkdirsRPC::Response> Run();

 private:
  NameNodeContext* namenode_context_;
  const MkdirsRPC::Request& request_;
};

}  // namespace rocketfs
