#pragma once

#include <memory>

#include "common/time_util.h"
#include "namenode/common/id_gen.h"
#include "namenode/table/inode_id.h"
#include "namenode/table/kv/kv_store_base.h"

namespace rocketfs {

class NameNodeCtx {
 public:
  NameNodeCtx();
  NameNodeCtx(const NameNodeCtx&) = delete;
  NameNodeCtx(NameNodeCtx&&) = delete;
  NameNodeCtx& operator=(const NameNodeCtx&) = delete;
  NameNodeCtx& operator=(NameNodeCtx&&) = delete;
  ~NameNodeCtx() = default;

  void Start();
  void Stop();

  KVStoreBase* GetKVStore();
  TimeUtilBase* GetTimeUtil();
  InodeIDGen& GetInodeIDGen();

 private:
  std::unique_ptr<TimeUtilBase> time_util_;
  std::unique_ptr<KVStoreBase> kv_store_;
  IDGenMgr id_generator_manager_;
  std::unique_ptr<InodeIDGen> inode_id_generator_;
};

}  // namespace rocketfs
