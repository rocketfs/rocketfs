#pragma once

#include <string>
#include <string_view>

#include "namenode/common/shared_mutex_manager.h"
#include "namenode/path_resolver/path_resolver_base.h"

namespace rocketfs {

class PathResolver : public PathResolverBase {
 public:
  PathResolver();
  PathResolver(const PathResolver&) = delete;
  PathResolver(PathResolver&&) = delete;
  PathResolver& operator=(const PathResolver&) = delete;
  PathResolver& operator=(PathResolver&&) = delete;
  ~PathResolver() override = default;

  Status Resolve(
      std::span<ToBeResolvedPath> to_be_resolved_paths,
      std::vector<std::vector<PathComponent>>* resolved_paths) override;

 private:
  //   std::pmr::memory_resource* memory_resource_;
  SharedMutexManager<std::string, std::string_view> path_lock_manager_;
};

}  // namespace rocketfs
