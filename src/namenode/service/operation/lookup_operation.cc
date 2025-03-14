// Copyright 2025 RocketFS

#include "namenode/service/operation/lookup_operation.h"

#include <sys/stat.h>

#include <coroutine>
#include <cstring>
#include <string>

#include <unifex/coroutine.hpp>

#include "common/status.h"

namespace rocketfs {

LookupOperation::LookupOperation(NameNodeContext* namenode_context,
                                 const LookupRPC::Request& request)
    : namenode_context_(namenode_context), request_(request) {
}

unifex::task<LookupRPC::Response> LookupOperation::Run() {
  if (request_.parent_inode_id() == 1 && request_.name() == "hello") {
    LookupRPC::Response response;
    response.set_inode_id(2);
    response.mutable_stat()->set_inode_id(2);
    response.mutable_stat()->set_mode(S_IFREG | 0444);
    response.mutable_stat()->set_nlink(1);
    response.mutable_stat()->set_uid(0);
    response.mutable_stat()->set_gid(0);
    response.mutable_stat()->set_size(strlen("Hello World!\n"));
    response.mutable_stat()->set_atime(0);
    response.mutable_stat()->set_mtime(0);
    response.mutable_stat()->set_ctime(0);
    response.mutable_stat()->set_block_size(4096);
    response.mutable_stat()->set_block_num(0);
    co_return response;
  } else {
    LookupRPC::Response response;
    response.set_error_code(static_cast<int>(StatusCode::kNotFoundError));
    co_return response;
  }
}

}  // namespace rocketfs
