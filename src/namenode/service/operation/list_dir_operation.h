// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_context.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using ListDirRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestListDir>;

class ListDirOperation {
 public:
  ListDirOperation(NameNodeContext* namenode_context,
                   const ListDirRPC::Request& request);
  ListDirOperation(const ListDirOperation&) = delete;
  ListDirOperation(ListDirOperation&&) = delete;
  ListDirOperation& operator=(const ListDirOperation&) = delete;
  ListDirOperation& operator=(ListDirOperation&&) = delete;
  ~ListDirOperation() = default;

  unifex::task<ListDirRPC::Response> Run();

 private:
  NameNodeContext* namenode_context_;
  const ListDirRPC::Request& request_;
};

}  // namespace rocketfs
