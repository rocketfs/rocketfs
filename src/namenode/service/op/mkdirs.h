// Copyright 2025 RocketFS

#pragma once

#include <grpcpp/support/status.h>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <agrpc/asio_grpc.hpp>
#include <unifex/let_value_with.hpp>

#include "common/logger.h"
#include "namenode/kv_store/kv_store_base.h"
#include "namenode/namenode_context.h"
#include "namenode/service/handler_context.h"
#include "namenode/transaction_manager/table/inode_table_base.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

using MkdirsRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestMkdirs>;
auto register_mkdirs_request_handler(
    agrpc::GrpcContext* grpc_context,
    ClientNamenodeService::AsyncService* service,
    NameNodeContext* namenode_context) {
  CHECK_NOTNULL(grpc_context);
  CHECK_NOTNULL(service);
  CHECK_NOTNULL(namenode_context);
  return agrpc::register_sender_rpc_handler<MkdirsRPC>(
      *grpc_context,
      *service,
      [namenode_context](MkdirsRPC& rpc, const MkdirsRPC::Request& request) {
        auto handler_context =
            std::make_unique<HandlerContext>(namenode_context);
        auto write_batch =
            handler_context->GetNameNodeContext()
                ->GetKVStore()
                ->CreateWriteBatch(handler_context->GetAllocator());
        INodeT inode;
        inode.parent_inode_id() = {16385};
        inode.name() = "test";
        handler_context->GetINodeTable()->Write(
            std::nullopt, inode, write_batch.get());
        handler_context->GetNameNodeContext()->GetKVStore()->Write(
            std::move(write_batch));
        return unifex::let_value_with([] { return MkdirsRPC::Response{}; },
                                      [&](auto& response) {
                                        return rpc.finish(response,
                                                          grpc::Status::OK);
                                      });
      });
}

}  // namespace rocketfs
