#pragma once

// IWYU pragma: no_include <list>
// IWYU pragma: no_include <map>
// IWYU pragma: no_include <set>

#include <expected>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "common/status.h"
#include "namenode/common/mutation.h"
#include "namenode/common/request_scoped_allocator.h"
#include "namenode/common/shared_mutex_manager.h"
#include "namenode/table/inode_table_base.h"

namespace rocketfs {

constexpr char kSeparator = '/';

struct ToBeResolvedPath {
  std::string_view path;
  InodeID inode_id;
  LockType last_component_lock_type;
};

struct PathComponent {
  std::string_view path;
  LockGuard<std::string, std::string_view>* lock_guard{nullptr};
  std::string_view last_inode_name;
  Mutation<Inode, InodeT>* last_inode{nullptr};
};

class ResolvedPath : public std::pmr::vector<PathComponent> {
 public:
  using allocator_type = RequestScopedAllocator;

  explicit ResolvedPath(allocator_type allocator);
  ResolvedPath(std::string_view path, allocator_type allocator);
  ResolvedPath(const ResolvedPath&) = delete;
  ResolvedPath(ResolvedPath&&) = default;
  ResolvedPath(ResolvedPath&& other, allocator_type allocator);
  ResolvedPath& operator=(const ResolvedPath&) = delete;
  ResolvedPath& operator=(ResolvedPath&&) = default;
  ~ResolvedPath() = default;

  std::string_view GetPath() const;
  void SetPath(std::string_view path);

 private:
  std::pmr::string path_;
};

struct ResolvedPaths : public std::pmr::vector<ResolvedPath> {
 public:
  using allocator_type = RequestScopedAllocator;

  explicit ResolvedPaths(allocator_type allocator);
  ResolvedPaths(const ResolvedPaths&) = delete;
  ResolvedPaths(ResolvedPaths&&) = default;
  ResolvedPaths& operator=(const ResolvedPaths&) = delete;
  ResolvedPaths& operator=(ResolvedPaths&&) = default;
  ~ResolvedPaths() = default;

  Mutation<Inode, InodeT>& AddInodeMutation(Mutation<Inode, InodeT>&& mutation);
  const std::pmr::vector<Mutation<Inode, InodeT>>& GetInodeMutations() const;

 private:
  std::pmr::vector<Mutation<Inode, InodeT>> inode_mutations_;
};

class PathResolverBase {
 public:
  PathResolverBase() = default;
  PathResolverBase(const PathResolverBase&) = delete;
  PathResolverBase(PathResolverBase&&) = delete;
  PathResolverBase& operator=(const PathResolverBase&) = delete;
  PathResolverBase& operator=(PathResolverBase&&) = delete;
  virtual ~PathResolverBase() = default;

  virtual std::expected<ResolvedPaths, Status> Resolve(
      std::span<ToBeResolvedPath> to_be_resolved_paths) = 0;
};

}  // namespace rocketfs
