#pragma once

#include <expected>
#include <functional>
#include <span>
#include <vector>

#include "common/status.h"
#include "namenode/common/request_scoped_allocator.h"
#include "namenode/path_resolver/path_resolver_base.h"
#include "namenode/table/inode_table_base.h"

namespace rocketfs {

struct PathComponentWithParent {
  PathComponent* parent;
  std::reference_wrapper<PathComponent> self;
};

class PathResolver : public PathResolverBase {
 public:
  PathResolver(InodeTableBase* inode_table, RequestScopedAllocator allocator);
  PathResolver(const PathResolver&) = delete;
  PathResolver(PathResolver&&) = delete;
  PathResolver& operator=(const PathResolver&) = delete;
  PathResolver& operator=(PathResolver&&) = delete;
  ~PathResolver() override = default;

  std::expected<ResolvedPaths, Status> Resolve(
      std::span<ToBeResolvedPath> to_be_resolved_paths) override;

 private:
  std::expected<ResolvedPaths, Status> BuildPathComponents(
      std::span<ToBeResolvedPath> to_be_resolved_paths);
  std::pmr::vector<PathComponentWithParent> SortPathComponents(
      ResolvedPaths* resolved_paths);
  void ResolveInodes(ResolvedPaths* resolved_paths,
                     std::span<PathComponentWithParent> path_components);

 private:
  InodeTableBase* inode_table_;
  RequestScopedAllocator allocator_;
};

}  // namespace rocketfs
