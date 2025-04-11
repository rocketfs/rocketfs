// Copyright 2025 RocketFS

#pragma once

namespace rocketfs {

class RpcServer {
 public:
  explicit RpcServer(int listening_port);
  RpcServer(const RpcServer&) = delete;
  RpcServer(RpcServer&&) = delete;
  RpcServer& operator=(const RpcServer&) = delete;
  RpcServer& operator=(RpcServer&&) = delete;
  ~RpcServer() = default;

  void Run();

 private:
  int sock_fd_;
  int epoll_fd_;
};

}  // namespace rocketfs
