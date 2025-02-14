// Copyright 2025 RocketFS

#include "namenode/namenode_context.h"

#include "namenode/kv_store/rocksdb_kv_store.h"

namespace rocketfs {

NameNodeContext::NameNodeContext()
    : kv_store_(std::make_unique<RocksDBKVStore>()) {
}

KVStoreBase* NameNodeContext::GetKVStore() {
  return kv_store_.get();
}

PathResolverBase* NameNodeContext::GetPathResolver() {
  return path_resolver_.get();
}

}  // namespace rocketfs
