// Copyright 2025 RocketFS

#pragma once

#include <grpcpp/completion_queue.h>

#include <optional>

#include "client/fuse/fuse_options.h"
#include "client/fuse/operation/fuse_async_operation_base.h"
#include "common/status.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

class FuseOpenDirOperation
    : public FuseAsyncOperationBase<ClientNamenodeService::Stub,
                                    ListDirRequest,
                                    ListDirResponse> {
 public:
  FuseOpenDirOperation(const FuseOptions& fuse_options,
                       fuse_req_t fuse_req,
                       fuse_ino_t inode_id,
                       struct fuse_file_info* file_info,
                       ClientNamenodeService::Stub* stub,
                       grpc::CompletionQueue* cq);

  void PrepareAsyncRpcCall();
  std::optional<int> ToErrno(StatusCode status_code);
  int OnSuccess();

 private:
  struct fuse_file_info* file_info_;
};

}  // namespace rocketfs
