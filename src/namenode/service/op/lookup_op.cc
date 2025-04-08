// Copyright 2025 RocketFS

#include "namenode/service/op/lookup_op.h"

#include <fmt/base.h>
#include <quill/LogMacros.h>
#include <quill/core/ThreadContextManager.h>
#include <sys/stat.h>

#include <coroutine>
#include <expected>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <unifex/coroutine.hpp>

#include "common/logger.h"
#include "common/status.h"
#include "namenode/service/handler_ctx.h"
#include "namenode/table/dent_view_base.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/file_table_base.h"
#include "namenode/table/hard_link_table_base.h"
#include "namenode/table/inode_id.h"

namespace rocketfs {

LookupOp::LookupOp(NameNodeCtx* namenode_ctx, const LookupRPC::Request& req)
    : OpBase(CHECK_NOTNULL(namenode_ctx)), req_(req) {
}

unifex::task<LookupRPC::Response> LookupOp::Run() {
  auto parent_id = InodeID{req_.parent_id()};
  auto parent_dir = co_await handler_ctx_.GetDirTable()->Read(parent_id);
  if (!parent_dir) {
    auto status = Status::SystemError(
        fmt::format("Unable to get parent inode {}.", parent_id.val),
        parent_dir.error());
    LOG_ERROR(logger, "{}", status.GetMsg());
    co_return status.MakeError<LookupRPC::Response>();
  }
  if (!*parent_dir) {
    Status status = Status::ParentNotFoundError(
        fmt::format("Parent inode {} not found.", parent_id.val));
    LOG_DEBUG(logger, "{}", status.GetMsg());
    co_return status.MakeError<LookupRPC::Response>();
  }
  auto has_permission = CheckPermission(
      (*parent_dir)->acl, User{req_.uid(), req_.gid()}, S_IXOTH);
  if (!has_permission) {
    auto status = Status::PermissionError(
        fmt::format("Permission denied on parent inode {}.", parent_id.val),
        has_permission.error());
    LOG_DEBUG(logger, "{}", status.GetMsg());
    co_return status.MakeError<LookupRPC::Response>();
  }

  auto dent = co_await handler_ctx_.GetDEntView()->Read(parent_id, req_.name());
  if (!dent) {
    auto status = Status::SystemError(
        fmt::format("Failed to look up with parent ID {} and name {}.",
                    req_.name(),
                    parent_id.val),
        dent.error());
    LOG_ERROR(logger, "{}", status.GetMsg());
    co_return status.MakeError<LookupRPC::Response>();
  }

  if (std::holds_alternative<std::monostate>(*dent)) {
    auto status = Status::NotFoundError(fmt::format(
        "Parent ID {} and name {} not found.", parent_id.val, req_.name()));
    LOG_DEBUG(logger, "{}", status.GetMsg());
    co_return status.MakeError<LookupRPC::Response>();
  }

  if (std::holds_alternative<Dir>(*dent)) {
    const auto& dir = std::get<Dir>(*dent);
    LookupRPC::Response resp;
    resp.set_id(dir.id.val);
    resp.mutable_stat()->set_id(dir.id.val);
    resp.mutable_stat()->set_mode(S_IFDIR | dir.acl.perm);
    resp.mutable_stat()->set_nlink(1);
    resp.mutable_stat()->set_uid(dir.acl.uid);
    resp.mutable_stat()->set_gid(dir.acl.gid);
    resp.mutable_stat()->set_atime_in_ns(dir.atime_in_ns);
    resp.mutable_stat()->set_mtime_in_ns(dir.mtime_in_ns);
    resp.mutable_stat()->set_ctime_in_ns(dir.ctime_in_ns);
    co_return resp;
  }

  CHECK(std::holds_alternative<HardLink>(*dent));
  const auto& hard_link = std::get<HardLink>(*dent);
  auto file = co_await handler_ctx_.GetFileTable()->Read(hard_link.id);
  if (!file) {
    auto status = Status::SystemError(
        fmt::format("Failed to get file {}.", hard_link.id.val), file.error());
    LOG_ERROR(logger, "{}", status.GetMsg());
    co_return status.MakeError<LookupRPC::Response>();
  }
  CHECK_NOTNULLOPT(*file);
  LookupRPC::Response resp;
  resp.set_id(hard_link.id.val);
  resp.mutable_stat()->set_id(hard_link.id.val);
  resp.mutable_stat()->set_mode(S_IFREG | (*file)->acl.perm);
  resp.mutable_stat()->set_nlink((*file)->nlink);
  resp.mutable_stat()->set_uid((*file)->acl.uid);
  resp.mutable_stat()->set_gid((*file)->acl.gid);
  resp.mutable_stat()->set_size((*file)->len);
  resp.mutable_stat()->set_atime_in_ns((*file)->atime_in_ns);
  resp.mutable_stat()->set_mtime_in_ns((*file)->mtime_in_ns);
  resp.mutable_stat()->set_ctime_in_ns((*file)->ctime_in_ns);
  resp.mutable_stat()->set_block_size((*file)->block_size);
  resp.mutable_stat()->set_block_num((*file)->blocks.size());
  co_return resp;
}

}  // namespace rocketfs
