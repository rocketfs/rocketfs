// Copyright 2025 RocketFS

#include "client/fuse/fuse_ops_proxy.h"

#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/status.h>
#include <quill/LogMacros.h>
#include <quill/core/ThreadContextManager.h>

#include <functional>
#include <memory>
#include <utility>

#include "common/logger.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

FuseOpsProxy::FuseOpsProxy(FuseOptions fuse_options)
    : fuse_options_(std::move(fuse_options)),
      namenode_channel_(CHECK_NOTNULL(grpc::CreateChannel(
          fuse_options_.server_endpoint, grpc::InsecureChannelCredentials()))),
      namenode_stub_(
          CHECK_NOTNULL(ClientNamenodeService::NewStub(namenode_channel_))) {
}

void FuseOpsProxy::Init(void* /*userdata*/, struct fuse_conn_info* /*conn*/) {
  namenode_completion_thread_ =
      std::make_unique<std::thread>([this]() { this->AsyncCompleteRpc(); });

  grpc::ClientContext client_context;
  GetInodeRequest req;
  req.set_path(fuse_options_.remote_mountpoint);
  GetInodeResponse resp;
  grpc::Status status = namenode_stub_->GetInode(&client_context, req, &resp);
  CHECK(status.ok());
  mountpoint_inode_id_ = resp.id();
}

void FuseOpsProxy::Destroy(void* /*userdata*/) {
  namenode_cq_.Shutdown();
  namenode_completion_thread_->join();

  mountpoint_inode_id_ = std::nullopt;
}

void FuseOpsProxy::AsyncCompleteRpc() {
  LOG_INFO(logger, "Starting to process RPC completions.");
  void* got_tag = nullptr;
  bool ok = false;
  while (namenode_cq_.Next(&got_tag, &ok)) {
    (*CHECK_NOTNULL(static_cast<std::function<void()>*>(got_tag)))();
    CHECK(ok);
  }
  LOG_INFO(logger, "Finished processing RPC completions.");
}

std::unique_ptr<FuseOpsProxy> gFuseOpsProxy;

}  // namespace rocketfs
