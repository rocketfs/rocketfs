// Copyright 2025 RocketFS

#include "namenode/path_resolver/path_resolver.h"

#include <fmt/base.h>

#include <algorithm>
#include <compare>
#include <cstddef>
#include <filesystem>  // NOLINT(build/c++17)
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "common/logger.h"
#include "namenode/common/mutation.h"

namespace rocketfs {

PathResolver::PathResolver(InodeTableBase* inode_table,
                           RequestScopedAllocator allocator)
    : inode_table_(inode_table), allocator_(allocator) {
  CHECK_NOTNULL(inode_table_);
}

std::expected<ResolvedPaths, Status> PathResolver::Resolve(
    std::span<ToBeResolvedPath> to_be_resolved_paths) {
  auto resolved_paths = BuildPathComponents(to_be_resolved_paths);
  if (!resolved_paths) {
    return std::unexpected(resolved_paths.error());
  }
  auto sorted_path_components = SortPathComponents(&*resolved_paths);
  ResolveInodes(&*resolved_paths, std::span{sorted_path_components});
  return resolved_paths;
}

std::expected<ResolvedPaths, Status> PathResolver::BuildPathComponents(
    std::span<ToBeResolvedPath> to_be_resolved_paths) {
  ResolvedPaths resolved_paths(allocator_);
  resolved_paths.resize(to_be_resolved_paths.size());
  for (size_t i = 0; i < to_be_resolved_paths.size(); i++) {
    const auto& to_be_resolved_path = to_be_resolved_paths[i];
    auto& resolved_path = resolved_paths.at(i);
    auto normalized_path = std::filesystem::path(to_be_resolved_path.path)
                               .lexically_normal()
                               .string();
    if (!(!normalized_path.empty() && normalized_path.front() == kSeparator &&
          (normalized_path.size() == 1 ||
           normalized_path.back() != kSeparator))) {
      return std::unexpected(Status::InvalidArgumentError(fmt::format(
          "{} is not a valid path. It must be non-empty and absolute.",
          to_be_resolved_path.path)));
    }
    resolved_path.SetPath(normalized_path);
    for (size_t pos = 0; pos < resolved_path.GetPath().size();) {
      auto next_pos = resolved_path.GetPath().find(kSeparator, pos + 1);
      auto current_path =
          resolved_path.GetPath().substr(0, std::max(next_pos, 1UL));
      auto last_inode_name = current_path.substr(pos + 1);
      resolved_path.emplace_back(
          current_path, nullptr, last_inode_name, nullptr);
      pos = next_pos;
    }
  }
  return resolved_paths;
}

std::pmr::vector<PathComponentWithParent> PathResolver::SortPathComponents(
    ResolvedPaths* resolved_paths) {
  CHECK_NOTNULL(resolved_paths);
  std::pmr::vector<PathComponentWithParent> all_components(allocator_);
  for (auto& resolved_path : *resolved_paths) {
    PathComponent* parent = nullptr;
    for (size_t i = 0; i < resolved_path.size(); i++) {
      auto& self = resolved_path.at(i);
      all_components.push_back({parent, self});
      parent = &self;
    }
  }
  std::stable_sort(all_components.begin(),
                   all_components.end(),
                   [](const auto& lhs, const auto& rhs) {
                     return lhs.self.get().path < rhs.self.get().path;
                   });
  return all_components;
}

void PathResolver::ResolveInodes(
    ResolvedPaths* resolved_paths,
    std::span<PathComponentWithParent> path_components) {
  CHECK_NOTNULL(resolved_paths);
  PathComponent* previous = nullptr;
  for (auto& component : path_components) {
    do {
      if (previous != nullptr && component.self.get().path == previous->path) {
        component.self.get().last_inode = previous->last_inode;
        break;
      }
      if (component.parent != nullptr && !*component.parent->last_inode) {
        component.self.get().last_inode = &resolved_paths->AddInodeMutation(
            Mutation<Inode, InodeT>(std::nullopt));
        break;
      }
      auto parent_id = component.parent == nullptr
                           ? kRootInodeID
                           : (*component.parent->last_inode)->inode_id();
      // TODO
      // auto inode =
      //     inode_table_->Read(parent_id,
      //     component.self.get().last_inode_name);
      // CHECK(static_cast<bool>(inode));
      // if (*inode) {
      //   component.self.get().last_inode = &resolved_paths->AddInodeMutation(
      //       Mutation<Inode, InodeT>(std::move(**inode)));
      // } else {
      //   component.self.get().last_inode = &resolved_paths->AddInodeMutation(
      //       Mutation<Inode, InodeT>(std::nullopt));
      // }
    } while (false);
    previous = &component.self.get();
  }
}

}  // namespace rocketfs
