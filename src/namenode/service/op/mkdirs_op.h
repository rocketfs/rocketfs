// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_ctx.h"
#include "namenode/service/op/op_base.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using MkdirsRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestMkdirs>;

class MkdirsOp : public OpBase<MkdirsRPC> {
 public:
  MkdirsOp(NameNodeCtx* namenode_ctx, const MkdirsRPC::Request& req);
  MkdirsOp(const MkdirsOp&) = delete;
  MkdirsOp(MkdirsOp&&) = delete;
  MkdirsOp& operator=(const MkdirsOp&) = delete;
  MkdirsOp& operator=(MkdirsOp&&) = delete;
  ~MkdirsOp() = default;

  unifex::task<MkdirsRPC::Response> Run();

 private:
  const MkdirsRPC::Request& req_;
};

}  // namespace rocketfs
