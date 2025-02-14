// Copyright 2025 RocketFS

#include "namenode/path_resolver/path_resolver.h"

// #include <fmt/format.h>

// #include <filesystem>

// #include "common/logger.h"

namespace rocketfs {

// Status PathResolver::Resolve(
//     std::span<ToBeResolvedPath> to_be_resolved_paths,
//     std::vector<std::vector<PathComponent>>* resolved_paths) {
//   CHECK_NOTNULL(resolved_paths);
//   CHECK(resolved_paths->empty());
//   std::pmr::vector<std::pmr::vector<PathComponent>> resolved_paths_pmr;
//   resolved_paths_pmr.resize(1);
//   resolved_paths->resize(to_be_resolved_paths.size());
//   for (auto& to_be_resolved_path : to_be_resolved_paths) {
//     auto& resolved_path = (*resolved_paths)[0];
//     auto normalized_path =
//         std::filesystem::path(to_be_resolved_path.path).lexically_normal();
//     if (normalized_path.empty() || !normalized_path.is_absolute()) {
//       return Status::InvalidArgumentError(
//           Status::OK(),
//           fmt::format("{} is not a non-empty absolute path.",
//                       to_be_resolved_path.path));
//     }
//     for (auto path_component = normalized_path; path_component != "/";
//          path_component = path_component.parent_path()) {
//       //   std::string(path_component.c_str());
//       //   std::pmr::string(path_component.filename().c_str(),
//       //                    resolved_paths->get_allocator());
//       //   resolved_path.emplace_back(
//       //       {.full_path = std::pmr::string(path_component.c_str(),
//       // resolved_paths->get_allocator()),
//       //        .last_inode_name =
//       //            std::pmr::string(path_component.filename().c_str(),
//       //                             resolved_paths->get_allocator())});
//     }
//     // auto lock = path_lock_manager_.AcquireLock(
//     //     to_be_resolved_path.path,
//     //     to_be_resolved_path.last_component_lock_type);
//   }
//   return Status::OK();
// }

}  // namespace rocketfs
