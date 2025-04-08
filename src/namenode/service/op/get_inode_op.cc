// Copyright 2025 RocketFS

#include "namenode/service/op/get_inode_op.h"

#include <fmt/base.h>
#include <quill/LogMacros.h>
#include <quill/core/ThreadContextManager.h>
#include <sys/stat.h>

#include <coroutine>
#include <expected>
#include <optional>
#include <string>
#include <utility>

#include <unifex/coroutine.hpp>

#include "common/logger.h"
#include "common/status.h"
#include "namenode/service/handler_ctx.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/inode_id.h"

namespace rocketfs {

GetInodeOp::GetInodeOp(NameNodeCtx* namenode_ctx,
                       const GetInodeRPC::Request& req)
    : OpBase(CHECK_NOTNULL(namenode_ctx)), req_(req) {
}

unifex::task<GetInodeRPC::Response> GetInodeOp::Run() {
  InodeID id = InodeID{req_.id()};
  auto dir = co_await handler_ctx_.GetDirTable()->Read(id);
  if (!dir) {
    auto status = Status::SystemError(
        fmt::format("Failed to get inode {}.", id.val), dir.error());
    LOG_ERROR(logger, "{}", status.GetMsg());
    co_return status.MakeError<GetInodeRPC::Response>();
  }
  if (!*dir) {
    auto status =
        Status::NotFoundError(fmt::format("Inode {} not found.", id.val));
    LOG_DEBUG(logger, "{}", status.GetMsg());
    co_return status.MakeError<GetInodeRPC::Response>();
  }
  GetInodeRPC::Response resp;
  resp.set_id((*dir)->id.val);
  resp.mutable_stat()->set_id((*dir)->id.val);
  resp.mutable_stat()->set_mode(S_IFDIR | (*dir)->acl.perm);
  resp.mutable_stat()->set_nlink(1);
  resp.mutable_stat()->set_uid((*dir)->acl.uid);
  resp.mutable_stat()->set_gid((*dir)->acl.gid);
  resp.mutable_stat()->set_atime_in_ns((*dir)->atime_in_ns);
  resp.mutable_stat()->set_mtime_in_ns((*dir)->mtime_in_ns);
  resp.mutable_stat()->set_ctime_in_ns((*dir)->ctime_in_ns);
  co_return resp;
}

}  // namespace rocketfs
