// Copyright 2025 RocketFS

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/support/status.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <agrpc/asio_grpc.hpp>
#include <unifex/blocking.hpp>
#include <unifex/detail/with_type_erased_tag_invoke.hpp>
#include <unifex/finally.hpp>
#include <unifex/inline_scheduler.hpp>
#include <unifex/just.hpp>
#include <unifex/just_from.hpp>
#include <unifex/overload.hpp>
#include <unifex/scheduler_concepts.hpp>
#include <unifex/sender_for.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/task.hpp>
#include <unifex/then.hpp>
#include <unifex/when_all.hpp>
#include <unifex/with_query_value.hpp>

#include "common/logger.h"
#include "namenode/namenode_ctx.h"
#include "namenode/service/operation/get_inode_op.h"
#include "namenode/service/operation/list_dir_op.h"
#include "namenode/service/operation/lookup_op.h"
#include "namenode/service/operation/mkdirs_op.h"
#include "namenode/service/operation/ping_pong_op.h"
#include "src/proto/client_namenode.grpc.pb.h"

namespace rocketfs {

template <typename Sender>
void RunGrpcCtxForSender(agrpc::GrpcContext* grpc_ctx, Sender&& sender) {
  CHECK_NOTNULL(grpc_ctx);
  grpc_ctx->work_started();
  unifex::sync_wait(unifex::when_all(
      unifex::finally(std::forward<Sender>(sender),
                      unifex::just_from([&] { grpc_ctx->work_finished(); })),
      unifex::just_from([&] { grpc_ctx->run(); })));
}

template <typename Rpc, typename Operation>
auto RegisterRpcHandler(agrpc::GrpcContext* grpc_ctx,
                        ClientNamenodeService::AsyncService* service,
                        NameNodeCtx* namenode_ctx) {
  return agrpc::register_sender_rpc_handler<Rpc>(
      *grpc_ctx,
      *service,
      [namenode_ctx](Rpc& rpc, const Rpc::Request& req) -> unifex::task<void> {
        auto resp = co_await Operation(namenode_ctx, req).Run();
        co_await rpc.finish(resp, grpc::Status::OK);
      });
}

}  // namespace rocketfs

int main(int argc, const char** argv) {
  const auto port = argc >= 2 ? argv[1] : "50051";
  const auto host = std::string("0.0.0.0:") + port;

  auto namenode_ctx = std::make_unique<rocketfs::NameNodeCtx>();
  namenode_ctx->Start();

  rocketfs::ClientNamenodeService::AsyncService service;
  std::unique_ptr<grpc::Server> server;

  grpc::ServerBuilder builder;
  agrpc::GrpcContext grpc_ctx{builder.AddCompletionQueue()};
  builder.AddListeningPort(host, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  server = builder.BuildAndStart();

  rocketfs::RunGrpcCtxForSender(
      &grpc_ctx,
      unifex::with_query_value(
          unifex::when_all(rocketfs::RegisterRpcHandler<rocketfs::PingPongRPC,
                                                        rocketfs::PingPongOp>(
                               &grpc_ctx, &service, namenode_ctx.get()),
                           rocketfs::RegisterRpcHandler<rocketfs::GetInodeRPC,
                                                        rocketfs::GetInodeOp>(
                               &grpc_ctx, &service, namenode_ctx.get()),
                           rocketfs::RegisterRpcHandler<rocketfs::LookupRPC,
                                                        rocketfs::LookupOp>(
                               &grpc_ctx, &service, namenode_ctx.get()),
                           rocketfs::RegisterRpcHandler<rocketfs::ListDirRPC,
                                                        rocketfs::ListDirOp>(
                               &grpc_ctx, &service, namenode_ctx.get()),
                           rocketfs::RegisterRpcHandler<rocketfs::MkdirsRPC,
                                                        rocketfs::MkdirsOp>(
                               &grpc_ctx, &service, namenode_ctx.get())),
          unifex::get_scheduler,
          unifex::inline_scheduler{}));
  namenode_ctx->Stop();
  return 0;
}
