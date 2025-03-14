// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_context.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using GetInodeRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestGetInode>;

class GetInodeOperation {
 public:
  GetInodeOperation(NameNodeContext* namenode_context,
                    const GetInodeRPC::Request& request);
  GetInodeOperation(const GetInodeOperation&) = delete;
  GetInodeOperation(GetInodeOperation&&) = delete;
  GetInodeOperation& operator=(const GetInodeOperation&) = delete;
  GetInodeOperation& operator=(GetInodeOperation&&) = delete;
  ~GetInodeOperation() = default;

  unifex::task<GetInodeRPC::Response> Run();

 private:
  NameNodeContext* namenode_context_;
  const GetInodeRPC::Request& request_;
};

}  // namespace rocketfs
