// Copyright 2025 RocketFS

#pragma once

#include <grpcpp/completion_queue.h>

#include "client/fuse/fuse_options.h"
#include "client/fuse/op/fuse_async_op_base.h"
#include "client/fuse/op/fuse_read_dir_op.h"
#include "src/proto/client_namenode.grpc.pb.h"

namespace rocketfs {

class FuseRelDirOp {
 public:
  FuseRelDirOp(const FuseOptions& fuse_options,
               fuse_req_t fuse_req,
               fuse_ino_t id,
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
