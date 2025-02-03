#pragma once

#include "namenode/kv_store/kv_store_base.h"
#include "namenode/namenode_context.h"

namespace rocketfs {

class HandlerContext {
 public:
  explicit HandlerContext(NameNodeContext* namenode_context);
  HandlerContext(const HandlerContext&) = delete;
  HandlerContext(HandlerContext&&) = delete;
  HandlerContext& operator=(const HandlerContext&) = delete;
  HandlerContext& operator=(HandlerContext&&) = delete;
  ~HandlerContext() = default;

  NameNodeContext* GetNameNodeContext();
  SnapshotBase* GetSnapshot();

 private:
  NameNodeContext* namenode_context_;
  std::unique_ptr<SnapshotBase> snapshot_;
};

}  // namespace rocketfs
