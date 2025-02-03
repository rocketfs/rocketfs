#include "namenode/service/handler_context.h"

#include "common/logger.h"

namespace rocketfs {

HandlerContext::HandlerContext(NameNodeContext* namenode_context)
    : namenode_context_(namenode_context),
      snapshot_(namenode_context->GetKVStore()->GetSnapshot()) {
  CHECK_NOTNULL(namenode_context_);
  CHECK_NOTNULL(snapshot_);
}

NameNodeContext* HandlerContext::GetNameNodeContext() {
  return namenode_context_;
}

SnapshotBase* HandlerContext::GetSnapshot() {
  return snapshot_.get();
}

}  // namespace rocketfs
