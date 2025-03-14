// Copyright 2025 RocketFS

#include "client/fuse/operation/fuse_read_dir_operation.h"

#include <google/protobuf/repeated_ptr_field.h>
#include <grpcpp/support/async_unary_call.h>
#include <quill/core/ThreadContextManager.h>
#include <sys/stat.h>

#include <memory>
#include <string>

#include "common/logger.h"

namespace rocketfs {

FuseReadDirOperation::FuseReadDirOperation(const FuseOptions& fuse_options,
                                           fuse_req_t fuse_req,
                                           fuse_ino_t inode_id,
                                           size_t size,
                                           off_t off,
                                           struct fuse_file_info* file_info,
                                           ClientNamenodeService::Stub* stub,
                                           grpc::CompletionQueue* cq)
    : FuseAsyncOperationBase(fuse_options, fuse_req, stub, cq),
      file_info_(CHECK_NOTNULL(file_info)),
      cache_dir_entries_(
          CHECK_NOTNULL(reinterpret_cast<CacheDirEntries*>(file_info->fh))),
      buf_holder_(size),
      buf_(buf_holder_) {
  static_assert(sizeof(off) == sizeof(cache_dir_entries_));
  CHECK_EQ(off, cache_dir_entries_->end_offset);
}

void FuseReadDirOperation::Start() {
  if (!cache_dir_entries_->entries.empty() || !cache_dir_entries_->has_more) {
    PopulateDirEntriesBuffer();
    LogReplyError(fuse_reply_buf(
        fuse_req_, buf_holder_.data(), buf_holder_.size() - buf_.size()));
    delete this;
    return;
  }
  FuseAsyncOperationBase<ClientNamenodeService::Stub,
                         ListDirRequest,
                         ListDirResponse>::Start();
}

void FuseReadDirOperation::PrepareAsyncRpcCall() {
  response_reader_ =
      stub_->PrepareAsyncListDir(&client_context_, request_, cq_);
}

std::optional<int> FuseReadDirOperation::ToErrno(StatusCode status_code) {
  // https://man7.org/linux/man-pages/man3/readdir.3.html
  // | `EBADF` | Invalid directory stream descriptor. |
  switch (status_code) {
    default:
      return std::nullopt;
  }
}

int FuseReadDirOperation::OnSuccess() {
  CHECK_EQ(response_.error_code(), 0);
  CHECK_EQ(cache_dir_entries_->end_offset, cache_dir_entries_->start_offset);
  CHECK(cache_dir_entries_->entries.empty());
  cache_dir_entries_->entries = {response_.entries().begin(),
                                 response_.entries().end()};
  cache_dir_entries_->has_more = response_.has_more();
  PopulateDirEntriesBuffer();
  return fuse_reply_buf(
      fuse_req_, buf_holder_.data(), buf_holder_.size() - buf_.size());
}

void FuseReadDirOperation::PopulateDirEntriesBuffer() {
  while (cache_dir_entries_->end_offset - cache_dir_entries_->start_offset <
         cache_dir_entries_->entries.size()) {
    const auto& entry = cache_dir_entries_->entries.at(
        cache_dir_entries_->end_offset - cache_dir_entries_->start_offset);
    // From the 'stbuf' argument the st_ino field and bits 12-15 of the st_mode
    // field are used. The other fields are ignored.
    struct stat stbuf;
    stbuf.st_ino = entry.inode_id();
    stbuf.st_mode = entry.mode();
    auto bytes_written = fuse_add_direntry(fuse_req_,
                                           buf_.data(),
                                           buf_.size(),
                                           entry.name().c_str(),
                                           &stbuf,
                                           cache_dir_entries_->end_offset + 1);
    if (bytes_written > buf_.size()) {
      break;
    }
    cache_dir_entries_->end_offset++;
    buf_ = buf_.subspan(bytes_written);
  }
  if (cache_dir_entries_->end_offset - cache_dir_entries_->start_offset ==
      cache_dir_entries_->entries.size()) {
    cache_dir_entries_->start_offset = cache_dir_entries_->end_offset;
    cache_dir_entries_->entries.clear();
  }
}

}  // namespace rocketfs
