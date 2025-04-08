// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_ctx.h"
#include "namenode/service/op/op_base.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using CreateRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestCreate>;

class CreateOp : public OpBase<CreateRPC> {
 public:
  CreateOp(NameNodeCtx* namenode_ctx, const CreateRPC::Request& req);
  CreateOp(const CreateOp&) = delete;
  CreateOp(CreateOp&&) = delete;
  CreateOp& operator=(const CreateOp&) = delete;
  CreateOp& operator=(CreateOp&&) = delete;
  ~CreateOp() = default;

  unifex::task<CreateRPC::Response> Run();

 private:
  const CreateRPC::Request& req_;
};

}  // namespace rocketfs
