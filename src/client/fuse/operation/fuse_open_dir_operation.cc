// Copyright 2025 RocketFS

#include "client/fuse/operation/fuse_open_dir_operation.h"

#include <errno.h>
#include <google/protobuf/repeated_ptr_field.h>
#include <grpcpp/support/async_unary_call.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "client/fuse/operation/fuse_read_dir_operation.h"
#include "common/logger.h"

namespace rocketfs {

FuseOpenDirOperation::FuseOpenDirOperation(const FuseOptions& fuse_options,
                                           fuse_req_t fuse_req,
                                           fuse_ino_t inode_id,
                                           struct fuse_file_info* file_info,
                                           ClientNamenodeService::Stub* stub,
                                           grpc::CompletionQueue* cq)
    : FuseAsyncOperationBase(fuse_options, fuse_req, stub, cq),
      file_info_(CHECK_NOTNULL(file_info)) {
  request_.set_inode_id(inode_id);
}

void FuseOpenDirOperation::PrepareAsyncRpcCall() {
  response_reader_ =
      stub_->PrepareAsyncListDir(&client_context_, request_, cq_);
}

std::optional<int> FuseOpenDirOperation::ToErrno(StatusCode status_code) {
  // https://man7.org/linux/man-pages/man3/opendir.3.html
  // | `EACCES`  | Permission denied.                                    |
  // | `EBADF`   | fd is not a valid file descriptor opened for reading. |
  // | `ENOENT`  | Directory does not exist, or name is an empty string. |
  // | `ENOTDIR` | name is not a directory.                              |
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

int FuseOpenDirOperation::OnSuccess() {
  auto cache_dir_entries = std::make_unique<CacheDirEntries>().release();
  cache_dir_entries->start_offset = 0;
  cache_dir_entries->end_offset = 0;
  cache_dir_entries->entries.emplace_back(response_.self_entry());
  cache_dir_entries->entries.emplace_back(response_.parent_entry());
  cache_dir_entries->entries.insert(cache_dir_entries->entries.end(),
                                    response_.entries().begin(),
                                    response_.entries().end());
  cache_dir_entries->has_more = response_.has_more();
  file_info_->fh = reinterpret_cast<uint64_t>(cache_dir_entries);
  return fuse_reply_open(fuse_req_, file_info_);
}

}  // namespace rocketfs
