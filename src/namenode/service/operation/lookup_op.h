// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_ctx.h"
#include "namenode/service/operation/op_base.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using LookupRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestLookup>;

class LookupOp : public OpBase<LookupRPC> {
 public:
  LookupOp(NameNodeCtx* namenode_ctx, const LookupRPC::Request& req);
  LookupOp(const LookupOp&) = delete;
  LookupOp(LookupOp&&) = delete;
  LookupOp& operator=(const LookupOp&) = delete;
  LookupOp& operator=(LookupOp&&) = delete;
  ~LookupOp() = default;

  unifex::task<LookupRPC::Response> Run();

 private:
  const LookupRPC::Request& req_;
};

}  // namespace rocketfs
