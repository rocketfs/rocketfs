// Copyright 2025 RocketFS

#include <gflags/gflags.h>
#include <google/protobuf/arena.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/support/status.h>

#include <memory_resource>
#include <type_traits>
#include <utility>

#include <agrpc/asio_grpc.hpp>
#include <agrpc/grpc_context.hpp>
#include <agrpc/register_sender_rpc_handler.hpp>
#include <agrpc/server_rpc.hpp>
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

#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

DEFINE_string(client_namenode_service_listen_address,
              "0.0.0.0:50051",
              "The listen address for the client namenode service.");

class ArenaRequestMessageFactory {
 public:
  ArenaRequestMessageFactory()
      : monotonic_buffer_resource_(&unsynchronized_pool_resource_),
        arena_(google::protobuf::ArenaOptions{
            .memory_resource = &monotonic_buffer_resource_}) {
  }

  template <class Request>
  Request& create() {
    return *google::protobuf::Arena::Create<Request>(&arena_);
  }

 private:
  std::pmr::unsynchronized_pool_resource unsynchronized_pool_resource_;
  std::pmr::monotonic_buffer_resource monotonic_buffer_resource_;
  // The goal is to allocate heap memory (from jemalloc/tcmalloc) only once
  // while serving an RPC request.
  google::protobuf::Arena arena_;
};

template <class Handler>
class RPCHandlerWithArenaRequestMessageFactory {
 public:
  explicit RPCHandlerWithArenaRequestMessageFactory(Handler handler)
      : handler_(std::move(handler)) {
  }

  template <class... Args>
  decltype(auto) operator()(Args&&... args) {
    return handler_(std::forward<Args>(args)...);
  }

  ArenaRequestMessageFactory request_message_factory() {
    return {};
  }

 private:
  Handler handler_;
};

auto register_ping_pong_request_handler(
    agrpc::GrpcContext* grpc_context,
    rocketfs::ClientNamenodeService::AsyncService* service) {
  using PingPongRPC = agrpc::ServerRPC<
      &rocketfs::ClientNamenodeService::AsyncService::Requestping_pong>;
  return agrpc::register_sender_rpc_handler<PingPongRPC>(
      *grpc_context,
      *service,
      RPCHandlerWithArenaRequestMessageFactory{
          [](PingPongRPC& rpc,
             const PingPongRPC::Request& /*request*/,
             ArenaRequestMessageFactory&) {
            return unifex::let_value_with(
                [&] { return PingPongRPC::Response(); },
                [&](auto& response) {
                  response.set_pong("pong");
                  return rpc.finish(response, grpc::Status::OK);
                });
          }});
}

template <class Sender>
void run_grpc_context_for_sender(agrpc::GrpcContext* grpc_context,
                                 Sender&& sender) {
  grpc_context->work_started();
  unifex::sync_wait(unifex::when_all(
      unifex::finally(std::forward<Sender>(sender), unifex::just_from([&] {
                        grpc_context->work_finished();
                      })),
      unifex::just_from([&] { grpc_context->run(); })));
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, /*remove_flags=*/true);
  rocketfs::ClientNamenodeService::AsyncService service;
  grpc::ServerBuilder builder;
  agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};
  builder.AddListeningPort(FLAGS_client_namenode_service_listen_address,
                           grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  auto server = builder.BuildAndStart();
  run_grpc_context_for_sender(
      &grpc_context,
      unifex::with_query_value(
          unifex::when_all(
              register_ping_pong_request_handler(&grpc_context, &service)),
          unifex::get_scheduler,
          unifex::inline_scheduler{}));
}
