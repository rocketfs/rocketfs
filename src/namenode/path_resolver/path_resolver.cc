#include "namenode/path_resolver/path_resolver.h"

#include <fmt/format.h>

#include <filesystem>

namespace rocketfs {

Status PathResolver::Resolve(
    std::span<ToBeResolvedPath> to_be_resolved_paths,
    std::pmr::vector<std::pmr::vector<PathComponent>>* resolved_paths) {
  for (auto& to_be_resolved_path : to_be_resolved_paths) {
    auto normalized_path =
        std::filesystem::path(to_be_resolved_path.path).lexically_normal();
    if (!normalized_path.is_absolute()) {
      return Status::InvalidArgumentError(
          Status::OK(),
          fmt::format("{} is not an absolute path.", to_be_resolved_path.path));
    }
    auto lock = path_lock_manager_.AcquireLock(
        to_be_resolved_path.path, to_be_resolved_path.last_component_lock_type);
  }
  return Status::OK();
}

}  // namespace rocketfs
