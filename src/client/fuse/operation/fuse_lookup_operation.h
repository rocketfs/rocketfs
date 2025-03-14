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

class FuseLookupOperation
    : public FuseAsyncOperationBase<ClientNamenodeService::Stub,
                                    LookupRequest,
                                    LookupResponse> {
 public:
  FuseLookupOperation(const FuseOptions& fuse_options,
                      fuse_req_t fuse_req,
                      fuse_ino_t parent_inode_id,
                      const char* name,
                      ClientNamenodeService::Stub* stub,
                      grpc::CompletionQueue* cq);

  void PrepareAsyncRpcCall();
  std::optional<int> ToErrno(StatusCode status_code);
  int OnSuccess();
};

}  // namespace rocketfs
