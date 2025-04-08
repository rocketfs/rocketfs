// Copyright 2025 RocketFS

#include "namenode/service/op/list_dir_op.h"

#include <fmt/base.h>
#include <gflags/gflags.h>
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
#include "namenode/table/hard_link_table_base.h"
#include "namenode/table/inode_id.h"

namespace rocketfs {

DECLARE_uint32(list_dir_default_limit);

ListDirOp::ListDirOp(NameNodeCtx* namenode_ctx, const ListDirRPC::Request& req)
    : OpBase(CHECK_NOTNULL(namenode_ctx)), req_(req) {
}

unifex::task<ListDirRPC::Response> ListDirOp::Run() {
  auto parent_id = InodeID{req_.id()};
  auto parent_dir = co_await handler_ctx_.GetDirTable()->Read(parent_id);
  if (!parent_dir) {
    auto status = Status::SystemError(
        fmt::format("Unable to get parent inode {}.", parent_id.val),
        parent_dir.error());
    LOG_ERROR(logger, "{}", status.GetMsg());
    co_return status.MakeError<ListDirRPC::Response>();
  }
  if (!*parent_dir) {
    Status status = Status::ParentNotFoundError(
        fmt::format("Parent inode {} not found.", parent_id.val));
    LOG_DEBUG(logger, "{}", status.GetMsg());
    co_return status.MakeError<ListDirRPC::Response>();
  }
  auto has_permission = CheckPermission(
      (*parent_dir)->acl, User{req_.uid(), req_.gid()}, S_IROTH);
  if (!has_permission) {
    auto status = Status::PermissionError(
        fmt::format("Permission denied on parent inode {}.", parent_id.val),
        has_permission.error());
    LOG_DEBUG(logger, "{}", status.GetMsg());
    co_return status.MakeError<ListDirRPC::Response>();
  }

  ListDirRPC::Response resp;
  const auto is_first_req = req_.start_after().empty();
  if (is_first_req) {
    resp.mutable_self_dent()->set_id(parent_id.val);
    resp.mutable_self_dent()->set_name(".");
    resp.mutable_self_dent()->set_type(S_IFDIR);

    if (parent_id == kRootInodeID) {
      resp.mutable_parent_dent()->set_id(parent_id.val);
      resp.mutable_parent_dent()->set_name("..");
      resp.mutable_parent_dent()->set_type(S_IFDIR);
    } else {
      auto grandparent_id = (*parent_dir)->parent_id;
      auto grandparent_dir =
          co_await handler_ctx_.GetDirTable()->Read(grandparent_id);
      if (!grandparent_dir) {
        Status status = Status::SystemError(
            fmt::format("Unable to get grandparent inode {}.",
                        grandparent_id.val),
            grandparent_dir.error());
        LOG_ERROR(logger, "{}", status.GetMsg());
        co_return status.MakeError<ListDirRPC::Response>();
      }
      if (*grandparent_dir) {
        resp.mutable_parent_dent()->set_id(grandparent_id.val);
        resp.mutable_parent_dent()->set_name("..");
        resp.mutable_parent_dent()->set_type(S_IFDIR);
      }
    }
  }

  auto ents = co_await handler_ctx_.GetDEntView()->List(
      parent_id,
      req_.start_after(),
      req_.limit() > 0 ? req_.limit() : FLAGS_list_dir_default_limit);
  if (!ents) {
    auto status = Status::SystemError(
        fmt::format("Unable to list dir {}.", parent_id.val), ents.error());
    LOG_ERROR(logger, "{}", status.GetMsg());
    co_return status.MakeError<ListDirRPC::Response>();
  }
  for (const auto& ent : ents.value()) {
    if (std::holds_alternative<Dir>(ent)) {
      auto dent = resp.add_ents();
      dent->set_id(std::get<Dir>(ent).id.val);
      dent->set_name(std::get<Dir>(ent).name);
      dent->set_type(S_IFDIR);
    } else if (std::holds_alternative<HardLink>(ent)) {
      auto dent = resp.add_ents();
      dent->set_id(std::get<HardLink>(ent).id.val);
      dent->set_name(std::get<HardLink>(ent).name);
      dent->set_type(S_IFREG);
    }
  }
  resp.set_has_more(ents.value().size() == req_.limit());
  co_return resp;
}

}  // namespace rocketfs
