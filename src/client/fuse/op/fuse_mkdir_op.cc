// Copyright 2025 RocketFS

#include "client/fuse/op/fuse_mkdir_op.h"

#include <errno.h>
#include <fcntl.h>  // IWYU pragma: keep
#include <grpcpp/support/async_unary_call.h>
#include <sys/stat.h>  // IWYU pragma: keep

#include <memory>

#include "common/logger.h"

namespace rocketfs {

FuseMkdirOp::FuseMkdirOp(const FuseOptions& fuse_options,
                         fuse_req_t fuse_req,
                         fuse_ino_t parent_id,
                         const char* name,
                         mode_t mode,
                         ClientNamenodeService::Stub* stub,
                         grpc::CompletionQueue* cq)
    : FuseAsyncOpBase(fuse_options, fuse_req, stub, cq) {
  req_.set_parent_id(parent_id);
  req_.set_name(name);
  req_.set_mode(mode | S_IFDIR);
}

void FuseMkdirOp::PrepareAsyncRpcCall() {
  resp_reader_ = stub_->PrepareAsyncMkdirs(&cli_ctx_, req_, cq_);
}

std::optional<int> FuseMkdirOp::ToErrno(StatusCode status_code) {
  // https://linux.die.net/man/2/mkdir
  // | `EACCES`  | Permission denied.                   |
  // | `EEXIST`  | Path already exists.                 |
  // | `ENOENT`  | The parent inode does not exist.     |
  // | `ENOTDIR` | The parent inode is not a dir.       |
  switch (status_code) {
    case StatusCode::kPermissionError:
      return EACCES;
    case StatusCode::kAlreadyExistsError:
      return EEXIST;
    case StatusCode::kParentNotFoundError:
      return ENOENT;
    case StatusCode::kParentNotDirError:
      return ENOTDIR;
    default:
      return std::nullopt;
  }
}

int FuseMkdirOp::OnSuccess() {
  CHECK_EQ(resp_.error_code(), 0);
  auto fuse_entry = ToFuseEntryParam(resp_.id(), resp_.stat());
  return fuse_reply_entry(fuse_req_, &fuse_entry);
}

}  // namespace rocketfs
