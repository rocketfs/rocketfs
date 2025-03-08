// Copyright 2025 RocketFS

#pragma once

#include <grpcpp/support/status.h>

#include <type_traits>

#include <agrpc/asio_grpc.hpp>
#include <unifex/let_value_with.hpp>

#include "common/logger.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

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

}  // namespace rocketfs
