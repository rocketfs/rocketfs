// Copyright 2025 RocketFS

#pragma once

#include <agrpc/asio_grpc.hpp>
#include <unifex/task.hpp>

#include "namenode/namenode_ctx.h"
#include "namenode/service/operation/op_base.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using ListDirRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestListDir>;

class ListDirOp : public OpBase<ListDirRPC> {
 public:
  ListDirOp(NameNodeCtx* namenode_ctx, const ListDirRPC::Request& req);
  ListDirOp(const ListDirOp&) = delete;
  ListDirOp(ListDirOp&&) = delete;
  ListDirOp& operator=(const ListDirOp&) = delete;
  ListDirOp& operator=(ListDirOp&&) = delete;
  ~ListDirOp() = default;

  unifex::task<ListDirRPC::Response> Run();

 private:
  const ListDirRPC::Request& req_;
};

}  // namespace rocketfs
