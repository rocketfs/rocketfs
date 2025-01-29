#pragma once

// IWYU pragma: no_include <list>
// IWYU pragma: no_include <map>
// IWYU pragma: no_include <set>

#include <memory>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "common/status.h"
#include "namenode/common/shared_mutex_manager.h"
#include "namenode/transaction_manager/table/inode_table_base.h"

namespace rocketfs {

struct ToBeResolvedPath {
  std::string_view path;
  INodeID inode_id;
  LockType last_component_lock_type;
};

class PathComponent {
 public:
  using allocator_type = std::pmr::polymorphic_allocator<void>;

  PathComponent(allocator_type allocator);
  PathComponent(const PathComponent& other) = delete;
  PathComponent(PathComponent&& other);
  PathComponent(PathComponent&& other, allocator_type allocator);
  PathComponent& operator=(const PathComponent& other) = delete;
  PathComponent& operator=(PathComponent&& other);

 private:
  allocator_type allocator_;
  std::pmr::string full_path_;
  LockGuard<std::string, std::string_view>* lock_guard_;
  std::pmr::string last_inode_name_;
  std::shared_ptr<INodeT> last_inode_;
};

class ResolvedPaths {
 public:
  using allocator_type = std::pmr::polymorphic_allocator<void>;

  ResolvedPaths(allocator_type allocator);
  ResolvedPaths(const ResolvedPaths& other) = delete;
  ResolvedPaths(ResolvedPaths&& other);
  ResolvedPaths& operator=(const ResolvedPaths& other) = delete;
  ResolvedPaths& operator=(ResolvedPaths&& other);

 private:
  allocator_type allocator_;
  std::pmr::vector<PathComponent> components_;
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
      std::vector<std::vector<PathComponent>>* resolved_paths) = 0;
};

}  // namespace rocketfs
