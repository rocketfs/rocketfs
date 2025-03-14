// Copyright 2025 RocketFS

#include "namenode/service/operation/get_inode_operation.h"

#include <sys/stat.h>

#include <coroutine>

#include <unifex/coroutine.hpp>

#include "common/status.h"

namespace rocketfs {

GetInodeOperation::GetInodeOperation(NameNodeContext* namenode_context,
                                     const GetInodeRPC::Request& request)
    : namenode_context_(namenode_context), request_(request) {
}

unifex::task<GetInodeRPC::Response> GetInodeOperation::Run() {
  GetInodeRPC::Response response;
  if (request_.inode_id() == 2) {
    response.set_inode_id(2);
    response.mutable_stat()->set_inode_id(2);
    response.mutable_stat()->set_mode(S_IFREG | 0444);
    response.mutable_stat()->set_nlink(1);
    response.mutable_stat()->set_uid(0);
  } else if (request_.inode_id() == 1) {
    response.set_inode_id(1);
    response.mutable_stat()->set_inode_id(1);
    response.mutable_stat()->set_mode(S_IFDIR | 0755);
    response.mutable_stat()->set_nlink(2);
  } else {
    response.set_error_code(static_cast<int>(StatusCode::kNotFoundError));
  }
  co_return response;
}

}  // namespace rocketfs
