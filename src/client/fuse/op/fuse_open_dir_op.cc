// Copyright 2025 RocketFS

#include "client/fuse/op/fuse_open_dir_op.h"

#include <errno.h>
#include <google/protobuf/repeated_ptr_field.h>
#include <grpcpp/support/async_unary_call.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "client/fuse/op/fuse_read_dir_op.h"
#include "common/logger.h"

namespace rocketfs {

FuseOpenDirOp::FuseOpenDirOp(const FuseOptions& fuse_options,
                             fuse_req_t fuse_req,
                             fuse_ino_t id,
                             struct fuse_file_info* file_info,
                             ClientNamenodeService::Stub* stub,
                             grpc::CompletionQueue* cq)
    : FuseAsyncOpBase(fuse_options, fuse_req, stub, cq),
      file_info_(CHECK_NOTNULL(file_info)) {
  req_.set_id(id);
}

void FuseOpenDirOp::PrepareAsyncRpcCall() {
  resp_reader_ = stub_->PrepareAsyncListDir(&cli_ctx_, req_, cq_);
}

std::optional<int> FuseOpenDirOp::ToErrno(StatusCode status_code) {
  // https://man7.org/linux/man-pages/man3/opendir.3.html
  // | `EACCES`  | Permission denied.                                    |
  // | `EBADF`   | fd is not a valid file descriptor opened for reading. |
  // | `ENOENT`  | Directory does not exist, or name is an empty string. |
  // | `ENOTDIR` | name is not a dir.                                    |
  switch (status_code) {
    case StatusCode::kPermissionError:
      return EACCES;
    case StatusCode::kNotFoundError:
      return ENOENT;
    case StatusCode::kNotDirError:
      return ENOTDIR;
    default:
      return std::nullopt;
  }
}

int FuseOpenDirOp::OnSuccess() {
  auto cache_dir_entries = std::make_unique<CacheDirEntries>().release();
  cache_dir_entries->start_off = 0;
  cache_dir_entries->end_off = 0;
  cache_dir_entries->entries.emplace_back(resp_.self_dent());
  cache_dir_entries->entries.emplace_back(resp_.parent_dent());
  cache_dir_entries->entries.insert(cache_dir_entries->entries.end(),
                                    resp_.ents().begin(),
                                    resp_.ents().end());
  cache_dir_entries->has_more = resp_.has_more();
  file_info_->fh = reinterpret_cast<uint64_t>(cache_dir_entries);
  return fuse_reply_open(fuse_req_, file_info_);
}

}  // namespace rocketfs
