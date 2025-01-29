// Copyright 2025 RocketFS

#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/status.h>
#include <src/proto/client_namenode.grpc.pb.h>

#include <chrono>
#include <coroutine>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <agrpc/asio_grpc.hpp>
#include <unifex/blocking.hpp>
#include <unifex/coroutine.hpp>
#include <unifex/detail/with_type_erased_tag_invoke.hpp>
#include <unifex/finally.hpp>
#include <unifex/inline_scheduler.hpp>
#include <unifex/just.hpp>
#include <unifex/just_from.hpp>
#include <unifex/overload.hpp>
#include <unifex/scheduler_concepts.hpp>
#include <unifex/sender_for.hpp>
#include <unifex/stop_when.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/task.hpp>
#include <unifex/then.hpp>
#include <unifex/unstoppable.hpp>
#include <unifex/when_all.hpp>
#include <unifex/with_query_value.hpp>

#include "src/proto/client_namenode.pb.h"

unifex::task<void> make_ping_pong_request(
    agrpc::GrpcContext& grpc_context,
    rocketfs::ClientNamenodeService::Stub& stub) {
  using RPC = agrpc::ClientRPC<
      &rocketfs::ClientNamenodeService::Stub::PrepareAsyncPingPong>;

  grpc::ClientContext client_context;
  client_context.set_deadline(std::chrono::system_clock::now() +
                              std::chrono::seconds(5));

  rocketfs::PingRequest request;
  rocketfs::PongResponse response;
  const auto status = co_await RPC::request(
      grpc_context, stub, client_context, request, response);

  //   abort_if_not(status.ok());
  std::cout << "Server streaming: " << response.pong() << '\n';
}

unifex::task<void> make_mkdirs_request(
    agrpc::GrpcContext& grpc_contxt,
    rocketfs::ClientNamenodeService::Stub& stub) {
  using RPC = agrpc::ClientRPC<
      &rocketfs::ClientNamenodeService::Stub::PrepareAsyncMkdirs>;
  grpc::ClientContext client_context;
  client_context.set_deadline(std::chrono::system_clock::now() +
                              std::chrono::seconds(5));
  rocketfs::MkdirsRequest request;
  rocketfs::MkdirsResponse response;
  const auto status = co_await RPC::request(
      grpc_contxt, stub, client_context, request, response);
  //   abort_if_not(status.ok());
}

using ServerStreamingClientRPC = agrpc::ClientRPC<
    &rocketfs::ClientNamenodeService::Stub::PrepareAsyncPingPong>;

struct ReadContext {
  rocketfs::PongResponse response;
  bool ok;
};

auto response_processor(rocketfs::PongResponse& response) {
  return [&](bool ok) {
    if (ok) {
      std::cout << "Server streaming: " << response.pong() << '\n';
    }
  };
}

auto with_deadline(agrpc::GrpcContext& grpc_context,
                   std::chrono::system_clock::time_point deadline) {
  return unifex::stop_when(unifex::then(
      agrpc::Alarm(grpc_context).wait(deadline), [](auto&&...) {}));
}

template <class Sender>
void run_grpc_context_for_sender(agrpc::GrpcContext& grpc_context,
                                 Sender&& sender) {
  grpc_context.work_started();
  unifex::sync_wait(unifex::when_all(
      unifex::finally(std::forward<Sender>(sender),
                      unifex::just_from([&] { grpc_context.work_finished(); })),
      unifex::just_from([&] { grpc_context.run(); })));
}

int main(int argc, const char** argv) {
  const auto port = argc >= 2 ? argv[1] : "50051";
  const auto host = std::string("localhost:") + port;

  const auto channel =
      grpc::CreateChannel(host, grpc::InsecureChannelCredentials());
  rocketfs::ClientNamenodeService::Stub stub{channel};
  agrpc::GrpcContext grpc_context;

  run_grpc_context_for_sender(
      grpc_context,
      unifex::with_query_value(
          unifex::when_all(make_mkdirs_request(grpc_context, stub)),
          unifex::get_scheduler,
          unifex::inline_scheduler{}));
}
