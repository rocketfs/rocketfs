// Copyright 2025 RocketFS

#include "namenode/service/operation/lookup_op.h"

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

#include <unifex/coroutine.hpp>

#include "common/logger.h"
#include "common/status.h"
#include "namenode/service/handler_ctx.h"
#include "namenode/table/dent_view_base.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/hard_link_table_base.h"
#include "namenode/table/inode_id.h"

namespace rocketfs {

LookupOp::LookupOp(NameNodeCtx* namenode_ctx, const LookupRPC::Request& req)
    : OpBase<LookupRPC>(CHECK_NOTNULL(namenode_ctx)), req_(req) {
}

unifex::task<LookupRPC::Response> LookupOp::Run() {
  auto parent_id = InodeID{req_.parent_id()};
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
  LookupRPC::Response resp;
  resp.set_id(hard_link.id.val);
  resp.mutable_stat()->set_id(hard_link.id.val);
  resp.mutable_stat()->set_mode(S_IFREG);
  resp.mutable_stat()->set_nlink(1);
  co_return resp;
}

}  // namespace rocketfs
