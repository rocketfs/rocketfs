// Copyright 2025 RocketFS

#include "client/fuse/operation/fuse_get_attr_op.h"

#include <errno.h>
#include <grpcpp/support/async_unary_call.h>

#include <memory>

#include "common/logger.h"

namespace rocketfs {

FuseGetAttrOp::FuseGetAttrOp(const FuseOptions& fuse_options,
                             fuse_req_t fuse_req,
                             fuse_ino_t id,
                             struct fuse_file_info* /*file_info*/,
                             ClientNamenodeService::Stub* stub,
                             grpc::CompletionQueue* cq)
    : FuseAsyncOpBase(fuse_options, fuse_req, stub, cq) {
  req_.set_id(id);
}

void FuseGetAttrOp::PrepareAsyncRpcCall() {
  resp_reader_ = stub_->PrepareAsyncGetInode(&cli_ctx_, req_, cq_);
}

std::optional<int> FuseGetAttrOp::ToErrno(StatusCode status_code) {
  switch (status_code) {
    case StatusCode::kNotFoundError:
      return ENOENT;
    default:
      return std::nullopt;
  }
}

int FuseGetAttrOp::OnSuccess() {
  CHECK_EQ(resp_.error_code(), 0);
  auto linux_stat = ToLinuxStat(resp_.stat());
  return fuse_reply_attr(
      fuse_req_, &linux_stat, fuse_options_.attr_timeout_sec);
}

}  // namespace rocketfs
