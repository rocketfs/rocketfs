// Copyright 2025 RocketFS

#pragma once

#include <grpcpp/completion_queue.h>

#include "client/fuse/fuse_options.h"
#include "client/fuse/operation/fuse_async_operation_base.h"
#include "client/fuse/operation/fuse_read_dir_operation.h"
#include "src/proto/client_namenode.grpc.pb.h"

namespace rocketfs {

class FuseReleaseDirOperation {
 public:
  FuseReleaseDirOperation(const FuseOptions& fuse_options,
                          fuse_req_t fuse_req,
                          fuse_ino_t inode_id,
                          struct fuse_file_info* file_info,
                          ClientNamenodeService::Stub* stub,
                          grpc::CompletionQueue* cq);

  void Start();

 private:
  fuse_req_t fuse_req_;
  struct fuse_file_info* file_info_;
  CacheDirEntries* cache_dir_entries_;
};

}  // namespace rocketfs
