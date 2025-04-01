#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>

#include "namenode/common/req_scoped_alloc.h"
#include "namenode/namenode_ctx.h"
#include "namenode/table/dent_view_base.h"
#include "namenode/table/dir_table_base.h"
#include "namenode/table/file_table_base.h"
#include "namenode/table/hard_link_table_base.h"
#include "namenode/table/kv/kv_store_base.h"

namespace rocketfs {

class HandlerCtx {
 public:
  explicit HandlerCtx(NameNodeCtx* namenode_ctx);
  HandlerCtx(const HandlerCtx&) = delete;
  HandlerCtx(HandlerCtx&&) = delete;
  HandlerCtx& operator=(const HandlerCtx&) = delete;
  HandlerCtx& operator=(HandlerCtx&&) = delete;
  ~HandlerCtx() = default;

  NameNodeCtx* GetCtx();
  std::unique_ptr<TxnBase> GetTxn();
  ReqScopedAlloc GetAlloc();
  DirTableBase* GetDirTable();
  FileTableBase* GetFileTable();
  HardLinkTableBase* GetHardLinkTable();
  DEntViewBase* GetDEntView();

 private:
  NameNodeCtx* namenode_ctx_;

  uint32_t request_monotonic_buffer_resource_prealloc_bytes_;
  std::unique_ptr<std::byte[]> memory_resource_holder_;
  std::pmr::monotonic_buffer_resource monotonic_buffer_resource_;
  ReqScopedAlloc alloc_;

  std::unique_ptr<TxnBase> txn_;
  std::unique_ptr<DirTableBase> dir_table_;
  std::unique_ptr<FileTableBase> file_table_;
  std::unique_ptr<HardLinkTableBase> hard_link_table_;
  std::unique_ptr<DEntViewBase> dent_view_;
};

}  // namespace rocketfs
