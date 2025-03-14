// Copyright 2025 RocketFS

#include "namenode/service/operation/list_dir_operation.h"

#include <sys/stat.h>

#include <coroutine>

#include <unifex/coroutine.hpp>

#include "common/status.h"

namespace rocketfs {

ListDirOperation::ListDirOperation(NameNodeContext* namenode_context,
                                   const ListDirRPC::Request& request)
    : namenode_context_(namenode_context), request_(request) {
}

unifex::task<ListDirRPC::Response> ListDirOperation::Run() {
  ListDirRPC::Response response;
  if (request_.inode_id() != 1) {
    response.set_error_code(static_cast<int>(StatusCode::kNotFoundError));
    co_return response;
  }
  response.mutable_self_entry()->set_inode_id(1);
  response.mutable_self_entry()->set_name(".");
  response.mutable_self_entry()->set_mode(S_IFDIR | 0755);
  response.mutable_parent_entry()->set_inode_id(1);
  response.mutable_parent_entry()->set_name("..");
  response.mutable_parent_entry()->set_mode(S_IFDIR | 0755);
  auto dir_entry = response.add_entries();
  dir_entry->set_inode_id(2);
  dir_entry->set_name("hello");
  dir_entry->set_mode(S_IFREG | 0444);
  response.set_has_more(false);
  co_return response;
}

}  // namespace rocketfs
