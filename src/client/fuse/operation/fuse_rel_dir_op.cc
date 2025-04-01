// Copyright 2025 RocketFS

#include "client/fuse/operation/fuse_rel_dir_op.h"

#include "common/logger.h"

namespace rocketfs {

FuseRelDirOp::FuseRelDirOp(const FuseOptions& fuse_options,
                           fuse_req_t fuse_req,
                           fuse_ino_t id,
                           struct fuse_file_info* file_info,
                           ClientNamenodeService::Stub* stub,
                           grpc::CompletionQueue* cq)
    : fuse_req_(CHECK_NOTNULL(fuse_req)),
      file_info_(CHECK_NOTNULL(file_info)),
      cache_dir_entries_(
          CHECK_NOTNULL(reinterpret_cast<CacheDirEntries*>(file_info->fh))) {
}

void FuseRelDirOp::Start() {
  delete cache_dir_entries_;
  fuse_reply_err(fuse_req_, 0);
  delete this;
}

}  // namespace rocketfs
