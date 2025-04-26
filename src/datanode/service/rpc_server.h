// Copyright 2025 RocketFS

#pragma once

namespace rocketfs {

// https://www.reddit.com/r/cpp/comments/13dmuqt/coroutine_made_dpdk_dev_easy/
// https://github.com/alibaba/PhotonLibOS
// https://man.openbsd.org/kqueue.2
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
