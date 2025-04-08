// Copyright 2025 RocketFS

#include "namenode/service/op/mkdirs_op.h"

#include <fmt/base.h>
#include <quill/LogMacros.h>
#include <quill/core/ThreadContextManager.h>
#include <sys/stat.h>

#include <coroutine>
#include <expected>
#include <memory_resource>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include <unifex/coroutine.hpp>

#include "common/logger.h"
#include "common/status.h"
#include "common/time_util.h"
#include "namenode/service/handler_ctx.h"
#include "namenode/service/op/op_base.h"
#include "namenode/table/dent_view_base.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/inode_id.h"
#include "namenode/table/kv/kv_store_base.h"

namespace rocketfs {

MkdirsOp::MkdirsOp(NameNodeCtx* namenode_ctx, const MkdirsRPC::Request& req)
    : OpBase(CHECK_NOTNULL(namenode_ctx)), req_(req) {
}

unifex::task<MkdirsRPC::Response> MkdirsOp::Run() {
  auto parent_id = InodeID{req_.parent_id()};
  auto parent_dir = co_await handler_ctx_.GetDirTable()->Read(parent_id);
  if (!parent_dir) {
    auto status = Status::SystemError(
        fmt::format("Failed to get parent dir for inode {}.", parent_id.val),
        parent_dir.error());
    LOG_ERROR(logger, "{}", status.GetMsg());
    co_return status.MakeError<MkdirsRPC::Response>();
  }
  if (!*parent_dir) {
    auto status = Status::ParentNotFoundError(
        fmt::format("Parent dir {} not found.", parent_id.val));
    LOG_DEBUG(logger, "{}", status.GetMsg());
    co_return status.MakeError<MkdirsRPC::Response>();
  }
  auto has_permission = CheckPermission(
      (*parent_dir)->acl, User{req_.uid(), req_.gid()}, S_IWOTH);
  if (!has_permission) {
    auto status = Status::PermissionError(
        fmt::format("Permission denied on parent inode {}.", parent_id.val),
        has_permission.error());
    LOG_DEBUG(logger, "{}", status.GetMsg());
    co_return status.MakeError<MkdirsRPC::Response>();
  }

  auto dent = co_await handler_ctx_.GetDEntView()->Read(parent_id, req_.name());
  if (!dent) {
    auto status = Status::SystemError(
        fmt::format("Failed to verify if the dir entry with parent ID {} and "
                    "name {} exists.",
                    parent_id.val,
                    req_.name()));
    LOG_ERROR(logger, "{}", status.GetMsg());
    co_return status.MakeError<MkdirsRPC::Response>();
  }
  if (!std::holds_alternative<std::monostate>(*dent)) {
    auto status = Status::AlreadyExistsError(
        fmt::format("Dir entry with parent ID {} and name {} already exists.",
                    parent_id.val,
                    req_.name()));
    LOG_DEBUG(logger, "{}", status.GetMsg());
    co_return status.MakeError<MkdirsRPC::Response>();
  }

  auto id = handler_ctx_.GetCtx()->GetInodeIDGen().Next();
  Acl acl{.uid = req_.uid(), .gid = req_.gid(), .perm = req_.mode() & ALLPERMS};
  if ((*parent_dir)->acl.perm & S_ISGID) {
    acl.gid = (*parent_dir)->acl.gid;
    acl.perm |= S_ISGID;
  }
  auto now_ns = handler_ctx_.GetCtx()->GetTimeUtil()->NowNs();
  Dir dir{.parent_id = InodeID{req_.parent_id()},
          .name = std::pmr::string(req_.name(), handler_ctx_.GetAlloc()),
          .id = id,
          .acl = acl,
          .ctime_in_ns = now_ns,
          .mtime_in_ns = now_ns,
          .atime_in_ns = now_ns};
  handler_ctx_.GetDirTable()->Write(std::nullopt, dir);
  co_await handler_ctx_.GetCtx()->GetKVStore()->CommitTxn(
      handler_ctx_.GetTxn());
  MkdirsRPC::Response resp;
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

}  // namespace rocketfs
