// Copyright 2025 RocketFS

#pragma once

#define FUSE_USE_VERSION 312

#include <fuse3/fuse_lowlevel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/impl/call.h>
#include <grpcpp/support/status.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "client/fuse/fuse_options.h"
#include "common/logger.h"
#include "common/status.h"
#include "common/time_util.h"
#include "src/proto/client_namenode.grpc.pb.h"
#include "src/proto/client_namenode.pb.h"

namespace rocketfs {

template <typename Stub, typename Rep, typename Resp>
class FuseAsyncOpBase {
 public:
  FuseAsyncOpBase(const FuseAsyncOpBase&) = delete;
  FuseAsyncOpBase(FuseAsyncOpBase&&) = delete;
  FuseAsyncOpBase& operator=(const FuseAsyncOpBase&) = delete;
  FuseAsyncOpBase& operator=(FuseAsyncOpBase&&) = delete;
  ~FuseAsyncOpBase() = default;

  void Start(this auto&& self);
  void Finish(this auto&& self);

 protected:
  FuseAsyncOpBase(const FuseOptions& fuse_options,
                  fuse_req_t fuse_req,
                  Stub* stub,
                  grpc::CompletionQueue* cq);

  struct fuse_entry_param ToFuseEntryParam(uint64_t id, const Stat& stat) const;
  struct stat ToLinuxStat(const Stat& stat) const;
  void LogReplyError(int reply_errno);

 protected:
  const FuseOptions fuse_options_;
  fuse_req_t fuse_req_;

  Stub* stub_;
  grpc::CompletionQueue* cq_;

  grpc::ClientContext cli_ctx_;
  Rep req_;
  Resp resp_;
  grpc::Status status_;
  std::unique_ptr<grpc::ClientAsyncResponseReader<Resp>> resp_reader_;

  std::function<void()> on_finished_;
};

template <typename Stub, typename Rep, typename Resp>
FuseAsyncOpBase<Stub, Rep, Resp>::FuseAsyncOpBase(
    const FuseOptions& fuse_options,
    fuse_req_t fuse_req,
    Stub* stub,
    grpc::CompletionQueue* cq)
    : fuse_options_(fuse_options),
      fuse_req_(CHECK_NOTNULL(fuse_req)),
      stub_(CHECK_NOTNULL(stub)),
      cq_(CHECK_NOTNULL(cq)) {
}

template <typename Stub, typename Rep, typename Resp>
void FuseAsyncOpBase<Stub, Rep, Resp>::Start(this auto&& self) {
  self.PrepareAsyncRpcCall();
  self.resp_reader_->StartCall();
  self.on_finished_ = [&self]() { self.Finish(); };
  self.resp_reader_->Finish(&self.resp_, &self.status_, &self.on_finished_);
}

template <typename Stub, typename Rep, typename Resp>
void FuseAsyncOpBase<Stub, Rep, Resp>::Finish(this auto&& self) {
  if (!self.status_.ok()) {
    LOG_ERROR(logger,
              "Failed to get a resp for req {}. Status: {}.",
              self.req_.ShortDebugString(),
              self.status_.error_message());
    return;
  }

  if constexpr (requires { self.resp_.error_code(); }) {
    static_assert(requires { self.resp_.error_msg(); });
    if (self.resp_.error_code() != 0) {
      LOG_DEBUG(logger,
                "Received resp {} contains an error for req {}.",
                self.resp_.ShortDebugString(),
                self.req_.ShortDebugString());
      std::optional<int> error_code =
          self.ToErrno(static_cast<StatusCode>(self.resp_.error_code()));
      if (!error_code) {
        LOG_ERROR(logger,
                  "Received resp {} contains an unknown error for req {}.",
                  self.resp_.ShortDebugString(),
                  self.req_.ShortDebugString());
      }
      self.LogReplyError(
          fuse_reply_err(self.fuse_req_, error_code.value_or(EIO)));
      return;
    }
  }

  LOG_DEBUG(logger,
            "Received resp {} for req {}.",
            self.resp_.ShortDebugString(),
            self.req_.ShortDebugString());
  self.LogReplyError(self.OnSuccess());

  delete &self;
}

template <typename Stub, typename Rep, typename Resp>
struct fuse_entry_param FuseAsyncOpBase<Stub, Rep, Resp>::ToFuseEntryParam(
    uint64_t id, const Stat& stat) const {
  struct fuse_entry_param fuse_entry;
  fuse_entry.ino = id;
  fuse_entry.generation = fuse_options_.entry_generation;
  fuse_entry.attr = ToLinuxStat(stat);
  fuse_entry.attr_timeout = fuse_options_.attr_timeout_sec;
  fuse_entry.entry_timeout = fuse_options_.entry_timeout_sec;
  return fuse_entry;
}

template <typename Stub, typename Rep, typename Resp>
struct stat FuseAsyncOpBase<Stub, Rep, Resp>::ToLinuxStat(
    const Stat& stat) const {
  // https://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
  // The structure `stat` contains at least the following members:
  // | `st_ino`     | file serial number                             |
  // | `st_mode`    | mode of file                                   |
  // | `st_nlink`   | number of links to the file                    |
  // | `st_uid`     | user ID of file                                |
  // | `st_gid`     | group ID of file                               |
  // | `st_size`    | file size in bytes (if file is a regular file) |
  // | `st_atime`   | time of last access                            |
  // | `st_mtime`   | time of last data modification                 |
  // | `st_ctime`   | time of last status change                     |
  // | `st_blksize` | a filesystem-specific preferred I/O block size |
  // | `st_blocks`  | number of blocks allocated for this object     |
  struct stat linux_stat;
  linux_stat.st_ino = stat.id();
  linux_stat.st_mode = stat.mode();
  linux_stat.st_nlink = stat.nlink();
  linux_stat.st_uid = stat.uid();
  linux_stat.st_gid = stat.gid();
  linux_stat.st_size = stat.size();
  linux_stat.st_atime = stat.atime_in_ns() / kSecToNs;
  linux_stat.st_mtime = stat.mtime_in_ns() / kSecToNs;
  linux_stat.st_ctime = stat.ctime_in_ns() / kSecToNs;
  linux_stat.st_blksize = stat.block_size();
  linux_stat.st_blocks = stat.block_num();
  return linux_stat;
}

template <typename Stub, typename Rep, typename Resp>
void FuseAsyncOpBase<Stub, Rep, Resp>::LogReplyError(int reply_errno) {
  // `fuse_reply_*`, such as `fuse_reply_err`, returns `-errno`.
  // https://github.com/libfuse/libfuse/blob/master/include/fuse_lowlevel.h#L1349
  reply_errno = -1 * reply_errno;
  if (reply_errno != 0) {
    LOG_ERROR(logger,
              "Failed to reply for req {}. Errno: {}.",
              req_.ShortDebugString(),
              reply_errno);
  }
}

}  // namespace rocketfs
