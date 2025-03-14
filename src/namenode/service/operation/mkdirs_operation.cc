// Copyright 2025 RocketFS

#include "namenode/service/operation/mkdirs_operation.h"

#include <coroutine>
#include <expected>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <unifex/coroutine.hpp>

#include "common/logger.h"
#include "common/time_util.h"
#include "generated/inode_generated.h"
#include "namenode/common/mutation.h"
#include "namenode/common/shared_mutex_manager.h"
#include "namenode/kv_store/kv_store_base.h"
#include "namenode/path_resolver/path_resolver_base.h"
#include "namenode/service/handler_context.h"
#include "namenode/table/inode_table_base.h"

namespace rocketfs {

MkdirsOperation::MkdirsOperation(NameNodeContext* namenode_context,
                                 const MkdirsRPC::Request& request)
    : namenode_context_(namenode_context), request_(request) {
  CHECK_NOTNULL(namenode_context_);
}

unifex::task<MkdirsRPC::Response> MkdirsOperation::Run() {
  HandlerContext handler_context(namenode_context_);
  ToBeResolvedPath path{request_.src(), kInvalidInodeID, LockType::kWrite};
  auto resolved_paths =
      handler_context.GetPathResolver()->Resolve(std::span{&path, 1});
  CHECK_EQ(resolved_paths->size(), 1);
  auto& resolved_path = resolved_paths->at(0);
  InodeID parent_inode_id = kInvalidInodeID;
  for (auto& path_component : resolved_path) {
    if (*path_component.last_inode) {
      continue;
    }
    path_component.last_inode->Create();
    auto& last_inode = **path_component.last_inode;
    last_inode.inode_id() =
        handler_context.GetNameNodeContext()->GetInodeIDGenerator().Next();
    last_inode.inode_type_union().Set(
        serde::DirectoryT{.parent_inode_id = parent_inode_id.index,
                          .name = std::string(path_component.last_inode_name)});
    last_inode.ctime_in_nanoseconds() = handler_context.GetNameNodeContext()
                                            ->GetTimeUtil()
                                            ->GetCurrentTimeInNanoseconds();
    last_inode.mtime_in_nanoseconds() = last_inode.ctime_in_nanoseconds();
    last_inode.atime_in_nanoseconds() = last_inode.ctime_in_nanoseconds();
    parent_inode_id = last_inode.inode_id();
  }
  handler_context.GetInodeTable()->Write(*resolved_paths);
  co_await handler_context.GetNameNodeContext()
      ->GetKVStore()
      ->CommitTransaction(handler_context.GetTransaction());
  co_return MkdirsRPC::Response{};
}

}  // namespace rocketfs
