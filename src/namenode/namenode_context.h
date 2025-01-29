#pragma once

#include <memory>

#include "namenode/kv_store/kv_store_base.h"
#include "namenode/path_resolver/path_resolver_base.h"

namespace rocketfs {

class NameNodeContext {
 public:
  NameNodeContext();
  NameNodeContext(const NameNodeContext&) = delete;
  NameNodeContext(NameNodeContext&&) = delete;
  NameNodeContext& operator=(const NameNodeContext&) = delete;
  NameNodeContext& operator=(NameNodeContext&&) = delete;
  ~NameNodeContext() = default;

  KVStoreBase* GetKVStore();
  PathResolverBase* GetPathResolver();

 private:
  std::unique_ptr<KVStoreBase> kv_store_;
  std::unique_ptr<PathResolverBase> path_resolver_;
};

}  // namespace rocketfs
