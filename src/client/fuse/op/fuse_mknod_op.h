// Copyright 2025 RocketFS

#pragma once

#include <grpcpp/completion_queue.h>
#include <sys/types.h>

#include <optional>

#include "client/fuse/fuse_options.h"
#include "client/fuse/op/fuse_async_op_base.h"
#include "common/status.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

class FuseMknodOp : public FuseAsyncOpBase<ClientNamenodeService::Stub,
                                           CreateRequest,
                                           CreateResponse> {
 public:
  FuseMknodOp(const FuseOptions& fuse_options,
              fuse_req_t fuse_req,
              fuse_ino_t parent_id,
              const char* name,
              mode_t mode,
              dev_t rdev,
              ClientNamenodeService::Stub* stub,
              grpc::CompletionQueue* cq);

  void Start();
  void PrepareAsyncRpcCall();
  std::optional<int> ToErrno(StatusCode status_code);
  int OnSuccess();
};

}  // namespace rocketfs
