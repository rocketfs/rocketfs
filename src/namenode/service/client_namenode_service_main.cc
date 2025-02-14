// Copyright 2025 RocketFS

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <rocksdb/db.h>
#include <src/proto/client_namenode.grpc.pb.h>

#include <agrpc/asio_grpc.hpp>
#include <unifex/finally.hpp>
#include <unifex/just_from.hpp>
#include <unifex/just_void_or_done.hpp>
#include <unifex/let_value_with.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/task.hpp>
#include <unifex/then.hpp>
#include <unifex/when_all.hpp>
#include <unifex/with_query_value.hpp>

#include "common/logger.h"
#include "namenode/namenode_context.h"
#include "namenode/service/handler_context.h"

namespace rocketfs {

using PingPongRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestPingPong>;
auto register_ping_pong_request_handler(
    agrpc::GrpcContext* grpc_context,
    ClientNamenodeService::AsyncService* service) {
  CHECK_NOTNULL(grpc_context);
  CHECK_NOTNULL(service);
  return agrpc::register_sender_rpc_handler<PingPongRPC>(
      *grpc_context,
      *service,
      [&](PingPongRPC& rpc, const PingPongRPC::Request& request) {
        return unifex::let_value_with([] { return PingPongRPC::Response{}; },
                                      [&](auto& response) {
                                        response.set_pong("pong");
                                        return rpc.finish(response,
                                                          grpc::Status::OK);
                                      });
      });
}

using MkdirsRPC =
    agrpc::ServerRPC<&ClientNamenodeService::AsyncService::RequestMkdirs>;
auto register_mkdirs_request_handler(
    agrpc::GrpcContext* grpc_context,
    ClientNamenodeService::AsyncService* service,
    NameNodeContext* namenode_context) {
  CHECK_NOTNULL(grpc_context);
  CHECK_NOTNULL(service);
  CHECK_NOTNULL(namenode_context);
  auto handler_context = std::make_unique<HandlerContext>(namenode_context);
  return agrpc::register_sender_rpc_handler<MkdirsRPC>(
      *grpc_context,
      *service,
      [&](MkdirsRPC& rpc, const MkdirsRPC::Request& request) {
        auto write_batch =
            handler_context->GetNameNodeContext()
                ->GetKVStore()
                ->CreateWriteBatch(handler_context->GetMemoryResource());
        handler_context->GetNameNodeContext()->GetKVStore()->Write(
            std::move(write_batch));
        return unifex::let_value_with([] { return MkdirsRPC::Response{}; },
                                      [&](auto& response) {
                                        return rpc.finish(response,
                                                          grpc::Status::OK);
                                      });
      });
}

template <class Sender>
void run_grpc_context_for_sender(agrpc::GrpcContext* grpc_context,
                                 Sender&& sender) {
  CHECK_NOTNULL(grpc_context);
  grpc_context->work_started();
  unifex::sync_wait(unifex::when_all(
      unifex::finally(std::forward<Sender>(sender), unifex::just_from([&] {
                        grpc_context->work_finished();
                      })),
      unifex::just_from([&] { grpc_context->run(); })));
}

}  // namespace rocketfs

int main(int argc, const char** argv) {
  const auto port = argc >= 2 ? argv[1] : "50051";
  const auto host = std::string("0.0.0.0:") + port;

  auto namenode_context = std::make_unique<rocketfs::NameNodeContext>();

  rocketfs::ClientNamenodeService::AsyncService service;
  std::unique_ptr<grpc::Server> server;

  grpc::ServerBuilder builder;
  agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};
  builder.AddListeningPort(host, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  server = builder.BuildAndStart();

  quill::Backend::start();

  rocketfs::run_grpc_context_for_sender(
      &grpc_context,
      unifex::with_query_value(
          unifex::when_all(
              rocketfs::register_ping_pong_request_handler(&grpc_context,
                                                           &service),
              rocketfs::register_mkdirs_request_handler(
                  &grpc_context, &service, namenode_context.get())),
          unifex::get_scheduler,
          unifex::inline_scheduler{}));
}
