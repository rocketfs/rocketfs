// Copyright 2025 RocketFS

#include "namenode/table/dir_entry_table_base.h"

namespace rocketfs {

DirEntry::DirEntry(std::pmr::string dir_entry_str)
    : dir_entry_str_(std::move(dir_entry_str)) {
  std::span<const uint8_t> dir_entry_span(
      reinterpret_cast<uint8_t*>(dir_entry_str_.data()), dir_entry_str_.size());
  CHECK(flatbuffers::Verifier(dir_entry_span.data(), dir_entry_span.size())
            .VerifyBuffer<serde::DirEntry>());
  dir_entry_ = flatbuffers::GetRoot<serde::DirEntry>(dir_entry_span.data());
  CHECK_NOTNULL(dir_entry_);
}

InodeID DirEntry::parent_inode_id() const {
  return {dir_entry_->parent_inode_id()};
}

std::string_view DirEntry::name() const {
  return dir_entry_->name()->str();
}

InodeID DirEntry::inode_id() const {
  return {dir_entry_->inode_id()};
}

DirEntryT::DirEntryT(const DirEntryT& dir_entry) {
  dir_entry.dir_entry_->UnPackTo(&dir_entry_);
}

InodeID DirEntryT::parent_inode_id() const {
  return {dir_entry_.parent_inode_id()};
}

std::string_view DirEntryT::name() const {
  return dir_entry_.name()->str();
}

InodeID DirEntryT::inode_id() const {
  return {dir_entry_.inode_id()};
}

InodeID& DirEntryT::parent_inode_id() {
  static_assert(sizeof(InodeID) == sizeof(dir_entry_.parent_inode_id));
  return reinterpret_cast<InodeID&>(dir_entry_.parent_inode_id);
}

std::string& DirEntryT::name() {
  return *dir_entry_.name();
}

InodeID& DirEntryT::inode_id() {
  static_assert(sizeof(InodeID) == sizeof(dir_entry_.inode_id));
  return reinterpret_cast<InodeID&>(dir_entry_.inode_id);
}

}  // namespace rocketfs
