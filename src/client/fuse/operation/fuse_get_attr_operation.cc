// Copyright 2025 RocketFS

#include "client/fuse/operation/fuse_get_attr_operation.h"

#include <errno.h>
#include <grpcpp/support/async_unary_call.h>

#include <memory>

#include "common/logger.h"

namespace rocketfs {

FuseGetAttrOperation::FuseGetAttrOperation(const FuseOptions& fuse_options,
                                           fuse_req_t fuse_req,
                                           fuse_ino_t inode_id,
                                           struct fuse_file_info* /*file_info*/,
                                           ClientNamenodeService::Stub* stub,
                                           grpc::CompletionQueue* cq)
    : FuseAsyncOperationBase(fuse_options, fuse_req, stub, cq) {
  request_.set_inode_id(inode_id);
}

void FuseGetAttrOperation::PrepareAsyncRpcCall() {
  response_reader_ =
      stub_->PrepareAsyncGetInode(&client_context_, request_, cq_);
}

std::optional<int> FuseGetAttrOperation::ToErrno(StatusCode status_code) {
  switch (status_code) {
    case StatusCode::kNotFoundError:
      return ENOENT;
    default:
      return std::nullopt;
  }
}

int FuseGetAttrOperation::OnSuccess() {
  CHECK_EQ(response_.error_code(), 0);
  auto linux_stat = ToLinuxStat(response_.stat());
  return fuse_reply_attr(
      fuse_req_, &linux_stat, fuse_options_.attr_timeout_sec);
}

}  // namespace rocketfs
