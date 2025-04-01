// Copyright 2025 RocketFS

#include "namenode/namenode_ctx.h"

#include <memory>

#include "common/time_util.h"
#include "namenode/table/kv/rocksdb_kv_store.h"

namespace rocketfs {

NameNodeCtx::NameNodeCtx()
    : kv_store_(std::make_unique<RocksDBKVStore>()),
      time_util_(std::make_unique<TimeUtil>()),
      id_generator_manager_(time_util_.get()) {
}

void NameNodeCtx::Start() {
  inode_id_generator_ = std::make_unique<InodeIDGen>(0, 0, 0);
  id_generator_manager_.AddIDGenerator(inode_id_generator_.get());
  id_generator_manager_.Start();
}

void NameNodeCtx::Stop() {
  id_generator_manager_.Stop();
}

TimeUtilBase* NameNodeCtx::GetTimeUtil() {
  return time_util_.get();
}

KVStoreBase* NameNodeCtx::GetKVStore() {
  return kv_store_.get();
}

InodeIDGen& NameNodeCtx::GetInodeIDGen() {
  return *inode_id_generator_;
}

}  // namespace rocketfs
