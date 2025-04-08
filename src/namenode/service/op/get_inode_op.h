// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_ctx.h"
#include "namenode/service/op/op_base.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using GetInodeRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestGetInode>;

class GetInodeOp : public OpBase<GetInodeRPC> {
 public:
  GetInodeOp(NameNodeCtx* namenode_ctx, const GetInodeRPC::Request& req);
  GetInodeOp(const GetInodeOp&) = delete;
  GetInodeOp(GetInodeOp&&) = delete;
  GetInodeOp& operator=(const GetInodeOp&) = delete;
  GetInodeOp& operator=(GetInodeOp&&) = delete;
  ~GetInodeOp() = default;

  unifex::task<GetInodeRPC::Response> Run();

 private:
  const GetInodeRPC::Request& req_;
};

}  // namespace rocketfs
