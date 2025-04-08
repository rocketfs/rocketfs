// Copyright 2025 RocketFS

#include "namenode/service/handler_ctx.h"

#include <gflags/gflags.h>

#include <memory>
#include <utility>

#include "common/logger.h"
#include "namenode/table/kv/kv_dent_view.h"
#include "namenode/table/kv/kv_dir_table.h"
#include "namenode/table/kv/kv_file_table.h"
#include "namenode/table/kv/kv_hard_link_table.h"

namespace rocketfs {

DECLARE_uint32(request_monotonic_buffer_resource_prealloc_bytes);

HandlerCtx::HandlerCtx(NameNodeCtx* namenode_ctx)
    : namenode_ctx_(namenode_ctx),
      request_monotonic_buffer_resource_prealloc_bytes_(
          FLAGS_request_monotonic_buffer_resource_prealloc_bytes),
      memory_resource_holder_(std::make_unique<std::byte[]>(
          request_monotonic_buffer_resource_prealloc_bytes_)),
      monotonic_buffer_resource_(
          memory_resource_holder_.get(),
          request_monotonic_buffer_resource_prealloc_bytes_),
      alloc_(&monotonic_buffer_resource_),
      txn_(namenode_ctx_->GetKVStore()->StartTxn(alloc_)),
      dir_table_(std::make_unique<KVDirTable>(txn_.get(), alloc_)),
      file_table_(std::make_unique<KVFileTable>(txn_.get(), alloc_)),
      hard_link_table_(std::make_unique<KVHardLinkTable>(txn_.get(), alloc_)),
      dent_view_(std::make_unique<KVDEntView>(txn_.get(), alloc_)) {
  CHECK_NOTNULL(namenode_ctx_);
  CHECK_GT(request_monotonic_buffer_resource_prealloc_bytes_, 0);
  CHECK_NOTNULL(memory_resource_holder_);
  CHECK_NOTNULL(txn_);
  CHECK_NOTNULL(dir_table_);
  CHECK_NOTNULL(file_table_);
  CHECK_NOTNULL(hard_link_table_);
  CHECK_NOTNULL(dent_view_);
}

NameNodeCtx* HandlerCtx::GetCtx() {
  return namenode_ctx_;
}

std::unique_ptr<TxnBase> HandlerCtx::GetTxn() {
  return std::move(txn_);
}

ReqScopedAlloc HandlerCtx::GetAlloc() {
  return alloc_;
}

DirTableBase* HandlerCtx::GetDirTable() {
  return dir_table_.get();
}

FileTableBase* HandlerCtx::GetFileTable() {
  return file_table_.get();
}

HardLinkTableBase* HandlerCtx::GetHardLinkTable() {
  return hard_link_table_.get();
}

DEntViewBase* HandlerCtx::GetDEntView() {
  return dent_view_.get();
}

}  // namespace rocketfs
