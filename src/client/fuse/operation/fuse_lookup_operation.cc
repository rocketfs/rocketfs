// Copyright 2025 RocketFS

#include "client/fuse/operation/fuse_lookup_operation.h"

#include <errno.h>
#include <grpcpp/support/async_unary_call.h>

#include <memory>

#include "common/logger.h"
#include "common/status.h"

namespace rocketfs {

FuseLookupOperation::FuseLookupOperation(const FuseOptions& fuse_options,
                                         fuse_req_t fuse_req,
                                         fuse_ino_t parent_inode_id,
                                         const char* name,
                                         ClientNamenodeService::Stub* stub,
                                         grpc::CompletionQueue* cq)
    : FuseAsyncOperationBase(fuse_options, fuse_req, stub, cq) {
  request_.set_parent_inode_id(parent_inode_id);
  request_.set_name(name);
}

void FuseLookupOperation::PrepareAsyncRpcCall() {
  response_reader_ = stub_->PrepareAsyncLookup(&client_context_, request_, cq_);
}

std::optional<int> FuseLookupOperation::ToErrno(StatusCode status_code) {
  switch (status_code) {
    case StatusCode::kNotFoundError:
      return ENOENT;
    default:
      return std::nullopt;
  }
}

int FuseLookupOperation::OnSuccess() {
  CHECK_EQ(response_.error_code(), 0);
  auto fuse_entry = ToFuseEntryParam(response_.inode_id(), response_.stat());
  return fuse_reply_entry(fuse_req_, &fuse_entry);
}

}  // namespace rocketfs
