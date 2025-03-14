// Copyright 2025 RocketFS

#define FUSE_USE_VERSION 312

#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#include <grpcpp/channel.h>
#include <grpcpp/completion_queue.h>

#include <memory>
#include <optional>
#include <thread>
#include <utility>

#include "client/fuse/fuse_options.h"
#include "src/proto/client_namenode.grpc.pb.h"

#pragma once

namespace rocketfs {

class FuseOpsProxy {
 public:
  explicit FuseOpsProxy(FuseOptions fuse_options);
  FuseOpsProxy(const FuseOpsProxy&) = delete;
  FuseOpsProxy& operator=(const FuseOpsProxy&) = delete;
  FuseOpsProxy(FuseOpsProxy&&) = delete;
  FuseOpsProxy& operator=(FuseOpsProxy&&) = delete;
  ~FuseOpsProxy() = default;

  void Init(void* userdata, struct fuse_conn_info* conn);
  void Destroy(void* userdata);

  template <typename Operation, typename... Args>
  Operation* CreateOperation(Args&&... args);

 private:
  void AsyncCompleteRpc();

 private:
  const FuseOptions fuse_options_;
  const std::shared_ptr<grpc::Channel> namenode_channel_;
  std::unique_ptr<ClientNamenodeService::Stub> namenode_stub_;
  grpc::CompletionQueue namenode_cq_;
  std::unique_ptr<std::thread> namenode_completion_thread_;
  std::optional<fuse_ino_t> mountpoint_inode_id_;
};

template <typename Operation, typename... Args>
Operation* FuseOpsProxy::CreateOperation(Args&&... args) {
  auto operation = new Operation(fuse_options_,
                                 std::forward<Args>(args)...,
                                 namenode_stub_.get(),
                                 &namenode_cq_);
  operation->Start();
  return operation;
}

extern std::unique_ptr<FuseOpsProxy> gFuseOpsProxy;

}  // namespace rocketfs
