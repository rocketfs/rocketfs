// Copyright 2025 RocketFS

#include "namenode/namenode_context.h"

#include <memory>

#include "common/time_util.h"
#include "namenode/kv_store/rocksdb_kv_store.h"

namespace rocketfs {

NameNodeContext::NameNodeContext()
    : kv_store_(std::make_unique<RocksDBKVStore>()),
      time_util_(std::make_unique<TimeUtil>()),
      id_generator_manager_(time_util_.get()) {
}

void NameNodeContext::Start() {
  inode_id_generator_ = std::make_unique<InodeIDGenerator>(0, 0, 0);
  id_generator_manager_.AddIDGenerator(inode_id_generator_.get());
  id_generator_manager_.Start();
}

void NameNodeContext::Stop() {
  id_generator_manager_.Stop();
}

TimeUtilBase* NameNodeContext::GetTimeUtil() {
  return time_util_.get();
}

KVStoreBase* NameNodeContext::GetKVStore() {
  return kv_store_.get();
}

InodeIDGenerator& NameNodeContext::GetInodeIDGenerator() {
  return *inode_id_generator_;
}

}  // namespace rocketfs
