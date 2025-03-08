// Copyright 2025 RocketFS

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <quill/Backend.h>
#include <src/proto/client_namenode.grpc.pb.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <agrpc/asio_grpc.hpp>
#include <unifex/blocking.hpp>
#include <unifex/finally.hpp>
#include <unifex/inline_scheduler.hpp>
#include <unifex/just.hpp>
#include <unifex/just_from.hpp>
#include <unifex/let_value_with.hpp>
#include <unifex/scheduler_concepts.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/then.hpp>
#include <unifex/when_all.hpp>
#include <unifex/with_query_value.hpp>

#include "common/logger.h"
#include "namenode/namenode_context.h"
#include "namenode/service/op/mkdirs.h"
#include "namenode/service/op/ping_pong.h"

namespace rocketfs {

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
