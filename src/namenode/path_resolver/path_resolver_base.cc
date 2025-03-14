// Copyright 2025 RocketFS

#include "namenode/path_resolver/path_resolver_base.h"

#include <utility>

#include "common/logger.h"

namespace rocketfs {

ResolvedPath::ResolvedPath(allocator_type allocator)
    : std::pmr::vector<PathComponent>(allocator) {
}

ResolvedPath::ResolvedPath(std::string_view path, allocator_type allocator)
    : std::pmr::vector<PathComponent>(allocator), path_(path, allocator) {
}

// https://github.com/gcc-mirror/gcc/blob/952e17223d3a9809a32be23f86f77166b5860b36/libstdc%2B%2B-v3/include/bits/stl_vector.h#L353
ResolvedPath::ResolvedPath(ResolvedPath&& other, allocator_type allocator)
    : std::pmr::vector<PathComponent>(allocator) {
  CHECK_EQ(get_allocator(), other.get_allocator());
  static_cast<std::pmr::vector<PathComponent>>(*this) =
      std::move(static_cast<std::pmr::vector<PathComponent>>(other));
  path_ = std::move(other.path_);
}

std::string_view ResolvedPath::GetPath() const {
  return path_;
}

void ResolvedPath::SetPath(std::string_view path) {
  path_ = std::pmr::string(path, get_allocator());
}

ResolvedPaths::ResolvedPaths(allocator_type allocator)
    : std::pmr::vector<ResolvedPath>(allocator), inode_mutations_(allocator) {
}

Mutation<Inode, InodeT>& ResolvedPaths::AddInodeMutation(
    Mutation<Inode, InodeT>&& mutation) {
  inode_mutations_.emplace_back(std::move(mutation));
  return inode_mutations_.back();
}

const std::pmr::vector<Mutation<Inode, InodeT>>&
ResolvedPaths::GetInodeMutations() const {
  return inode_mutations_;
}

}  // namespace rocketfs
