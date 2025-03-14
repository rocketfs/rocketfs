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
#include "namenode/namenode_context.h"
#include "namenode/service/operation/get_inode_operation.h"
#include "namenode/service/operation/list_dir_operation.h"
#include "namenode/service/operation/lookup_operation.h"
#include "namenode/service/operation/mkdirs_operation.h"
#include "namenode/service/operation/ping_pong_operation.h"
#include "src/proto/client_namenode.grpc.pb.h"

namespace rocketfs {

template <typename Sender>
void RunGrpcContextForSender(agrpc::GrpcContext* grpc_context,
                             Sender&& sender) {
  CHECK_NOTNULL(grpc_context);
  grpc_context->work_started();
  unifex::sync_wait(unifex::when_all(
      unifex::finally(std::forward<Sender>(sender), unifex::just_from([&] {
                        grpc_context->work_finished();
                      })),
      unifex::just_from([&] { grpc_context->run(); })));
}

template <typename Rpc, typename Operation>
auto RegisterRpcHandler(agrpc::GrpcContext* grpc_context,
                        ClientNamenodeService::AsyncService* service,
                        NameNodeContext* namenode_context) {
  return agrpc::register_sender_rpc_handler<Rpc>(
      *grpc_context,
      *service,
      [namenode_context](Rpc& rpc,
                         const Rpc::Request& request) -> unifex::task<void> {
        auto response = co_await Operation(namenode_context, request).Run();
        co_await rpc.finish(response, grpc::Status::OK);
      });
}

}  // namespace rocketfs

int main(int argc, const char** argv) {
  const auto port = argc >= 2 ? argv[1] : "50051";
  const auto host = std::string("0.0.0.0:") + port;

  auto namenode_context = std::make_unique<rocketfs::NameNodeContext>();
  namenode_context->Start();

  rocketfs::ClientNamenodeService::AsyncService service;
  std::unique_ptr<grpc::Server> server;

  grpc::ServerBuilder builder;
  agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};
  builder.AddListeningPort(host, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  server = builder.BuildAndStart();

  rocketfs::RunGrpcContextForSender(
      &grpc_context,
      unifex::with_query_value(
          unifex::when_all(
              rocketfs::RegisterRpcHandler<rocketfs::PingPongRPC,
                                           rocketfs::PingPongOperation>(
                  &grpc_context, &service, namenode_context.get()),
              rocketfs::RegisterRpcHandler<rocketfs::GetInodeRPC,
                                           rocketfs::GetInodeOperation>(
                  &grpc_context, &service, namenode_context.get()),
              rocketfs::RegisterRpcHandler<rocketfs::LookupRPC,
                                           rocketfs::LookupOperation>(
                  &grpc_context, &service, namenode_context.get()),
              rocketfs::RegisterRpcHandler<rocketfs::ListDirRPC,
                                           rocketfs::ListDirOperation>(
                  &grpc_context, &service, namenode_context.get()),
              rocketfs::RegisterRpcHandler<rocketfs::MkdirsRPC,
                                           rocketfs::MkdirsOperation>(
                  &grpc_context, &service, namenode_context.get())),
          unifex::get_scheduler,
          unifex::inline_scheduler{}));
  namenode_context->Stop();
  return 0;
}
