// Copyright 2025 RocketFS

#pragma once

#include <grpcpp/completion_queue.h>
#include <stddef.h>
#include <sys/types.h>

#include <optional>
#include <span>
#include <vector>

#include "client/fuse/fuse_options.h"
#include "client/fuse/op/fuse_async_op_base.h"
#include "common/status.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

struct CacheDirEntries {
  // Inclusive starting position.
  off_t start_off;
  // Exclusive ending position (next position to read).
  off_t end_off;
  std::vector<ListDirResponse::DEnt> entries;
  bool has_more;
};

class FuseReadDirOp : public FuseAsyncOpBase<ClientNamenodeService::Stub,
                                             ListDirRequest,
                                             ListDirResponse> {
 public:
  FuseReadDirOp(const FuseOptions& fuse_options,
                fuse_req_t fuse_req,
                fuse_ino_t id,
                size_t size,
                off_t off,
                struct fuse_file_info* file_info,
                ClientNamenodeService::Stub* stub,
                grpc::CompletionQueue* cq);

  void Start();

  void PrepareAsyncRpcCall();
  std::optional<int> ToErrno(StatusCode status_code);
  int OnSuccess();
  void PopulateDirEntriesBuffer();

 private:
  struct fuse_file_info* file_info_;
  CacheDirEntries* cache_dir_entries_;
  std::vector<char> buf_holder_;
  std::span<char> buf_;
};

}  // namespace rocketfs
