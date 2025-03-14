#pragma once

#include <memory>

#include "common/time_util.h"
#include "namenode/common/id_generator.h"
#include "namenode/kv_store/kv_store_base.h"
#include "namenode/table/inode_table_base.h"

namespace rocketfs {

class NameNodeContext {
 public:
  NameNodeContext();
  NameNodeContext(const NameNodeContext&) = delete;
  NameNodeContext(NameNodeContext&&) = delete;
  NameNodeContext& operator=(const NameNodeContext&) = delete;
  NameNodeContext& operator=(NameNodeContext&&) = delete;
  ~NameNodeContext() = default;

  void Start();
  void Stop();

  KVStoreBase* GetKVStore();
  TimeUtilBase* GetTimeUtil();
  InodeIDGenerator& GetInodeIDGenerator();

 private:
  std::unique_ptr<TimeUtilBase> time_util_;
  std::unique_ptr<KVStoreBase> kv_store_;
  IDGeneratorManager id_generator_manager_;
  std::unique_ptr<InodeIDGenerator> inode_id_generator_;
};

}  // namespace rocketfs
