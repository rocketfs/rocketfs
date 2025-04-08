// Copyright 2025 RocketFS

#include "client/fuse/op/fuse_lookup_op.h"

#include <errno.h>
#include <grpcpp/support/async_unary_call.h>

#include <memory>

#include "common/logger.h"
#include "common/status.h"

namespace rocketfs {

FuseLookupOp::FuseLookupOp(const FuseOptions& fuse_options,
                           fuse_req_t fuse_req,
                           fuse_ino_t parent_id,
                           const char* name,
                           ClientNamenodeService::Stub* stub,
                           grpc::CompletionQueue* cq)
    : FuseAsyncOpBase(fuse_options, fuse_req, stub, cq) {
  req_.set_parent_id(parent_id);
  req_.set_name(name);
}

void FuseLookupOp::PrepareAsyncRpcCall() {
  resp_reader_ = stub_->PrepareAsyncLookup(&cli_ctx_, req_, cq_);
}

std::optional<int> FuseLookupOp::ToErrno(StatusCode status_code) {
  switch (status_code) {
    case StatusCode::kNotFoundError:
      return ENOENT;
    default:
      return std::nullopt;
  }
}

int FuseLookupOp::OnSuccess() {
  CHECK_EQ(resp_.error_code(), 0);
  auto fuse_entry = ToFuseEntryParam(resp_.id(), resp_.stat());
  return fuse_reply_entry(fuse_req_, &fuse_entry);
}

}  // namespace rocketfs
