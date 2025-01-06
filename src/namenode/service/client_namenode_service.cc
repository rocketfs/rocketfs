// Copyright 2025 RocketFS

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
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

using ExampleService = rocketfs::ClientNamenodeService::AsyncService;

using UnaryRPC = agrpc::ServerRPC<&ExampleService::Requestping_pong>;

auto register_unary_request_handler(
    agrpc::GrpcContext& grpc_context,
    rocketfs::ClientNamenodeService::AsyncService& service) {
  // Register a handler for all incoming RPCs of this unary method until the
  // server is being shut down.
  return agrpc::register_sender_rpc_handler<UnaryRPC>(
      grpc_context,
      service,
      [&](UnaryRPC& rpc, const UnaryRPC::Request& request) {
        return unifex::let_value_with([] { return UnaryRPC::Response{}; },
                                      [&](auto& response) {
                                        response.set_pong("pong");
                                        return rpc.finish(response,
                                                          grpc::Status::OK);
                                      });
      });
}

template <typename T>
struct TypePrinter;

template <class Sender>
void run_grpc_context_for_sender(agrpc::GrpcContext& grpc_context,
                                 Sender&& sender) {
  static_assert(
      std::is_same_v<
          unifex::sender_single_value_result_t<std::remove_cvref_t<Sender>>,
          std::variant<std::tuple<>>>);
  // template <typename Sender>
  // using sender_single_value_return_type_t =
  // sender_value_types_t<Sender,single_overload,single_value_type>::type::type;
  //
  // template <
  //     typename Sender,
  //     template <typename...> class Variant,
  //     template <typename...> class Tuple>
  // using sender_value_types_t =
  //     typename sender_traits<Sender>::template value_types<Variant, Tuple>;
  //
  static_assert(!unifex::detail::_has_bulk_sender_types<Sender>);
  static_assert(unifex::detail::_has_sender_types<Sender>);
  static_assert(
      std::is_same_v<
          typename std::remove_cvref_t<Sender>::template value_types<
              unifex::single_overload,
              unifex::single_value_type>,
          typename unifex::detail::_sender_traits<std::remove_cvref_t<Sender>>::
              template value_types<unifex::single_overload,
                                   unifex::single_value_type>>);
  static_assert(
      std::is_same_v<typename std::remove_cvref_t<Sender>::template value_types<
                         unifex::single_overload,
                         unifex::single_value_type>::type::type,
                     std::variant<std::tuple<>>>);
  // std::variant<std::tuple<>>
  // TypePrinter<
  //     unifex::sender_single_value_return_type_t<std::remove_cvref_t<Sender>>>
  //     test;

  grpc_context.work_started();
  auto x = unifex::when_all(
      unifex::finally(std::forward<Sender>(sender),
                      unifex::just_from([&] { grpc_context.work_finished(); })),
      unifex::just_from([&] { grpc_context.run(); }));
  static_assert(std::is_same_v<
                unifex::sender_single_value_result_t<
                    std::remove_cvref_t<decltype(x)>>,
                std::tuple<std::variant<std::tuple<std::variant<std::tuple<>>>>,
                           std::variant<std::tuple<>>>>);
  unifex::sync_wait(std::move(x));
}

int main(int argc, const char** argv) {
  const auto port = argc >= 2 ? argv[1] : "50051";
  const auto host = std::string("0.0.0.0:") + port;

  rocketfs::ClientNamenodeService::AsyncService service;
  std::unique_ptr<grpc::Server> server;

  grpc::ServerBuilder builder;
  agrpc::GrpcContext grpc_context{builder.AddCompletionQueue()};
  builder.AddListeningPort(host, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  // agrpc::add_health_check_service(builder);
  server = builder.BuildAndStart();
  // agrpc::start_health_check_service(*server, grpc_context);

  static_assert(
      unifex::detail::_has_sender_types<decltype(register_unary_request_handler(
          grpc_context, service))>);
  static_assert(
      std::is_same_v<
          typename std::remove_cvref_t<decltype(register_unary_request_handler(
              grpc_context, service))>::
              template value_types<unifex::single_overload,
                                   unifex::single_value_type>::type::type,
          void>);
  using Sender = agrpc::detail::RPCHandlerSender<
      UnaryRPC,
      decltype([&](UnaryRPC& rpc, const UnaryRPC::Request& request) {
        return unifex::let_value_with([] { return UnaryRPC::Response{}; },
                                      [&](auto& response) {
                                        response.set_pong("pong");
                                        return rpc.finish(response,
                                                          grpc::Status::OK);
                                      });
      })>;
  static_assert(std::is_same_v<typename Sender::template value_types<
                                   unifex::single_overload,
                                   unifex::single_value_type>::type::type,
                               void>);

  run_grpc_context_for_sender(
      grpc_context,
      unifex::with_query_value(unifex::when_all(register_unary_request_handler(
                                   grpc_context, service)),
                               unifex::get_scheduler,
                               unifex::inline_scheduler{}));
}
