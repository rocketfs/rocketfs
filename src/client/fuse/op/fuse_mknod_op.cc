// Copyright 2025 RocketFS

#include "client/fuse/op/fuse_mknod_op.h"

#include <errno.h>
#include <grpcpp/support/async_unary_call.h>
#include <quill/LogMacros.h>
#include <quill/core/ThreadContextManager.h>
#include <sys/stat.h>

#include <memory>

#include "common/logger.h"

namespace rocketfs {

FuseMknodOp::FuseMknodOp(const FuseOptions& fuse_options,
                         fuse_req_t fuse_req,
                         fuse_ino_t parent_id,
                         const char* name,
                         mode_t mode,
                         dev_t /*rdev*/,
                         ClientNamenodeService::Stub* stub,
                         grpc::CompletionQueue* cq)
    : FuseAsyncOpBase<ClientNamenodeService::Stub,
                      CreateRequest,
                      CreateResponse>(fuse_options, fuse_req, stub, cq) {
  req_.set_parent_id(parent_id);
  req_.set_name(name);
  req_.set_mode(mode);
}

void FuseMknodOp::Start() {
  if ((req_.mode() & S_IFMT) != S_IFREG) {
    LOG_DEBUG(logger, "Mknod supports creating only regular files.");
    // https://man7.org/linux/man-pages/man2/mknod.2.html
    // EINVAL: mode requested creation of something other than a regular file.
    fuse_reply_err(fuse_req_, EINVAL);
    return;
  }
  FuseAsyncOpBase::Start();
}

void FuseMknodOp::PrepareAsyncRpcCall() {
  resp_reader_ = stub_->PrepareAsyncCreate(&cli_ctx_, req_, cq_);
}

std::optional<int> FuseMknodOp::ToErrno(StatusCode status_code) {
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

int FuseMknodOp::OnSuccess() {
  CHECK_EQ(resp_.error_code(), 0);
  auto fuse_entry = ToFuseEntryParam(resp_.id(), resp_.stat());
  return fuse_reply_entry(fuse_req_, &fuse_entry);
}

}  // namespace rocketfs
