#pragma once

#include <cstdint>
#include <memory>
#include <memory_resource>
#include <shared_mutex>
#include <span>
#include <string>
#include <unifex/async_mutex.hpp>
#include <vector>

#include "common/status.h"
#include "generated/inode_generated.h"
#include "namenode/common/shared_mutex_manager.h"
#include "namenode/transaction_manager/table/inode_table.h"
#include "namenode/transaction_manager/table/inode_table_base.h"

namespace rocketfs {

struct ToBeResolvedPath {
  std::string_view path;
  INodeID inode_id;
  LockType last_component_lock_type;
};

struct PathComponent {
  std::pmr::string name;
  std::shared_ptr<INodeT> inode;
  unifex::async_mutex lock;
};

class PathResolverBase {
 public:
  PathResolverBase() = default;
  PathResolverBase(const PathResolverBase&) = delete;
  PathResolverBase(PathResolverBase&&) = delete;
  PathResolverBase& operator=(const PathResolverBase&) = delete;
  PathResolverBase& operator=(PathResolverBase&&) = delete;
  virtual ~PathResolverBase() = default;

  virtual Status Resolve(
      std::span<ToBeResolvedPath> to_be_resolved_paths,
      std::pmr::vector<std::pmr::vector<PathComponent>>* resolved_paths) = 0;
};

}  // namespace rocketfs
