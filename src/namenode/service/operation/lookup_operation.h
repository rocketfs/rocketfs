// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_context.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using LookupRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestLookup>;

class LookupOperation {
 public:
  LookupOperation(NameNodeContext* namenode_context,
                  const LookupRPC::Request& request);
  LookupOperation(const LookupOperation&) = delete;
  LookupOperation(LookupOperation&&) = delete;
  LookupOperation& operator=(const LookupOperation&) = delete;
  LookupOperation& operator=(LookupOperation&&) = delete;
  ~LookupOperation() = default;

  unifex::task<LookupRPC::Response> Run();

 private:
  NameNodeContext* namenode_context_;
  const LookupRPC::Request& request_;
};

}  // namespace rocketfs
