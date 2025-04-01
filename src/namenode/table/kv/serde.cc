// Copyright 2025 RocketFS

#include "namenode/table/kv/serde.h"

#include <absl/base/internal/endian.h>
#include <flatbuffers/buffer.h>
#include <flatbuffers/flatbuffer_builder.h>
#include <flatbuffers/string.h>
#include <flatbuffers/verifier.h>

#include <cstddef>
#include <cstring>
#include <tuple>
#include <utility>

#include "common/logger.h"
#include "generated/dent_generated.h"
#include "generated/inode_generated.h"

namespace rocketfs {

constexpr auto kDefaultFlatBufferBuilderSize = 128;

FBAllocator::FBAllocator(ReqScopedAlloc alloc) : alloc_(alloc) {
}

uint8_t* FBAllocator::allocate(size_t size) {
  return alloc_.allocate(size);
}

void FBAllocator::deallocate(uint8_t* p, size_t size) {
  alloc_.deallocate(p, size);
}

DetachedBuffer::DetachedBuffer(flatbuffers::DetachedBuffer&& buffer)
    : buffer_(std::move(buffer)) {
}

DetachedBuffer::operator std::string_view() const {
  return std::string_view(reinterpret_cast<const char*>(buffer_.data()),
                          buffer_.size());
}

InodeSerde::InodeSerde(ReqScopedAlloc alloc) : alloc_(alloc), fb_alloc_(alloc) {
}

bool InodeSerde::IsKeyChanged(const Dir& orig, const Dir& mod) const {
  return orig.id != mod.id;
}

bool InodeSerde::IsKeyChanged(const File& orig, const File& mod) const {
  return orig.id != mod.id;
}

bool InodeSerde::IsValueChanged(const Dir& orig, const Dir& mod) const {
  return orig.parent_id != mod.parent_id || orig.name != mod.name ||
         orig.id != mod.id || orig.acl != mod.acl ||
         orig.ctime_in_ns != mod.ctime_in_ns;
}

bool InodeSerde::IsValueChanged(const File& orig, const File& mod) const {
  return orig.id != mod.id || orig.acl != mod.acl ||
         orig.ctime_in_ns != mod.ctime_in_ns;
}

std::pmr::string InodeSerde::SerKey(InodeID id) {
  std::pmr::string inode_id_str(alloc_);
  inode_id_str.resize(sizeof(InodeID));
  static_assert(sizeof(InodeID) == sizeof(int64_t));
  absl::big_endian::Store64(inode_id_str.data(), id.val);
  return inode_id_str;
}

std::pmr::string InodeSerde::SerKey(const Dir& dir) {
  return SerKey(dir.id);
}

std::pmr::string InodeSerde::SerKey(const File& file) {
  return SerKey(file.id);
}

DetachedBuffer InodeSerde::SerVal(const Dir& dir) {
  serde::DirT serde_dir;
  serde_dir.parent_id = dir.parent_id.val;
  serde_dir.name = dir.name;
  serde_dir.id = dir.id.val;
  serde_dir.ctime_in_ns = dir.ctime_in_ns;
  serde::InodeTypeUnion type;
  type.Set(std::move(serde_dir));
  serde::InodeT serde_inode;
  serde_inode.type = std::move(type);
  flatbuffers::FlatBufferBuilder fbb(kDefaultFlatBufferBuilderSize, &fb_alloc_);
  fbb.Finish(serde::Inode::Pack(fbb, &serde_inode));
  return DetachedBuffer(fbb.Release());
}

DetachedBuffer InodeSerde::SerVal(const File& file) {
  serde::FileT serde_file;
  serde_file.id = file.id.val;
  serde_file.ctime_in_ns = file.ctime_in_ns;
  serde::InodeTypeUnion type;
  type.Set(std::move(serde_file));
  serde::InodeT serde_inode;
  serde_inode.type = std::move(type);
  flatbuffers::FlatBufferBuilder fbb(kDefaultFlatBufferBuilderSize, &fb_alloc_);
  fbb.Finish(serde::Inode::Pack(fbb, &serde_inode));
  return DetachedBuffer(fbb.Release());
}

InodeID InodeSerde::DeKey(std::string_view key) {
  CHECK_EQ(key.size(), sizeof(InodeID));
  InodeID id;
  static_assert(sizeof(id) == sizeof(int64_t));
  id.val = absl::big_endian::Load64(key.data());
  return id;
}

std::variant<Dir, File> InodeSerde::DeVal(std::string_view val) {
  CHECK(flatbuffers::Verifier(reinterpret_cast<const uint8_t*>(val.data()),
                              val.size())
            .VerifyBuffer<serde::Inode>());
  auto inode = flatbuffers::GetRoot<serde::Inode>(
      reinterpret_cast<const uint8_t*>(val.data()));
  if (auto dir = inode->type_as_Dir()) {
    return Dir{
        .parent_id = InodeID{dir->parent_id()},
        .name =
            std::pmr::string(dir->name()->c_str(), dir->name()->size(), alloc_),
        .id = InodeID{dir->id()},
        .ctime_in_ns = dir->ctime_in_ns(),
    };
  }
  auto file = inode->type_as_File();
  CHECK_NOTNULL(file);
  return File{
      .id = InodeID{file->id()},
      .ctime_in_ns = file->ctime_in_ns(),
  };
}

MTimeSerde::MTimeSerde(ReqScopedAlloc alloc) : alloc_(alloc), fb_alloc_(alloc) {
}

bool MTimeSerde::IsKeyChanged(const Dir& orig, const Dir& mod) const {
  return orig.id != mod.id;
}

bool MTimeSerde::IsKeyChanged(const File& orig, const File& mod) const {
  return orig.id != mod.id;
}

bool MTimeSerde::IsValueChanged(const Dir& orig, const Dir& mod) const {
  return orig.mtime_in_ns != mod.mtime_in_ns;
}

bool MTimeSerde::IsValueChanged(const File& orig, const File& mod) const {
  return orig.mtime_in_ns != mod.mtime_in_ns;
}

std::pmr::string MTimeSerde::SerKey(InodeID id) {
  return InodeSerde(alloc_).SerKey(id);
}

std::pmr::string MTimeSerde::SerKey(const Dir& dir) {
  return SerKey(dir.id);
}

std::pmr::string MTimeSerde::SerKey(const File& file) {
  return SerKey(file.id);
}

std::pmr::string MTimeSerde::SerVal(int64_t mtime_in_ns) {
  std::pmr::string mtime_str(alloc_);
  mtime_str.resize(sizeof(int64_t));
  absl::big_endian::Store64(mtime_str.data(), mtime_in_ns);
  return mtime_str;
}

std::pmr::string MTimeSerde::SerVal(const Dir& dir) {
  return SerVal(dir.mtime_in_ns);
}

std::pmr::string MTimeSerde::SerVal(const File& file) {
  return SerVal(file.mtime_in_ns);
}

int64_t MTimeSerde::DeVal(std::string_view val) {
  CHECK_EQ(val.size(), sizeof(int64_t));
  return absl::big_endian::Load64(val.data());
}

ATimeSerde::ATimeSerde(ReqScopedAlloc alloc) : alloc_(alloc), fb_alloc_(alloc) {
}

bool ATimeSerde::IsKeyChanged(const Dir& orig, const Dir& mod) const {
  return orig.id != mod.id;
}

bool ATimeSerde::IsKeyChanged(const File& orig, const File& mod) const {
  return orig.id != mod.id;
}

bool ATimeSerde::IsValueChanged(const Dir& orig, const Dir& mod) const {
  return orig.atime_in_ns != mod.atime_in_ns;
}

bool ATimeSerde::IsValueChanged(const File& orig, const File& mod) const {
  return orig.atime_in_ns != mod.atime_in_ns;
}

std::pmr::string ATimeSerde::SerKey(InodeID id) {
  return InodeSerde(alloc_).SerKey(id);
}

std::pmr::string ATimeSerde::SerKey(const Dir& dir) {
  return SerKey(dir.id);
}

std::pmr::string ATimeSerde::SerKey(const File& file) {
  return SerKey(file.id);
}

std::pmr::string ATimeSerde::SerVal(int64_t atime_in_ns) {
  std::pmr::string atime_str(alloc_);
  atime_str.resize(sizeof(int64_t));
  absl::big_endian::Store64(atime_str.data(), atime_in_ns);
  return atime_str;
}

std::pmr::string ATimeSerde::SerVal(const Dir& dir) {
  return SerVal(dir.atime_in_ns);
}

std::pmr::string ATimeSerde::SerVal(const File& file) {
  return SerVal(file.atime_in_ns);
}

int64_t ATimeSerde::DeVal(std::string_view val) {
  CHECK_EQ(val.size(), sizeof(int64_t));
  return absl::big_endian::Load64(val.data());
}

DEntSerde::DEntSerde(ReqScopedAlloc alloc) : alloc_(alloc), fb_alloc_(alloc) {
}

bool DEntSerde::IsKeyChanged(const Dir& orig, const Dir& mod) const {
  return orig.parent_id != mod.parent_id || orig.name != mod.name;
}

bool DEntSerde::IsKeyChanged(const HardLink& orig, const HardLink& mod) const {
  return orig.parent_id != mod.parent_id || orig.name != mod.name;
}

bool DEntSerde::IsValueChanged(const Dir& orig, const Dir& mod) const {
  return orig.id != mod.id;
}

bool DEntSerde::IsValueChanged(const HardLink& orig,
                               const HardLink& mod) const {
  return orig.id != mod.id;
}

std::pmr::string DEntSerde::SerKey(InodeID parent_id, std::string_view name) {
  std::pmr::string key(alloc_);
  key.resize(sizeof(InodeID) + name.size());
  static_assert(sizeof(InodeID) == sizeof(int64_t));
  absl::big_endian::Store64(key.data(), parent_id.val);
  std::memcpy(key.data() + sizeof(InodeID), name.data(), name.size());
  return key;
}

std::pmr::string DEntSerde::SerKey(const Dir& dir) {
  return SerKey(dir.parent_id, dir.name);
}

std::pmr::string DEntSerde::SerKey(const HardLink& hard_link) {
  return SerKey(hard_link.parent_id, hard_link.name);
}

DetachedBuffer DEntSerde::SerVal(const Dir& dir) {
  serde::DirT serde_dir;
  serde_dir.parent_id = dir.parent_id.val;
  serde_dir.name = dir.name;
  serde_dir.id = dir.id.val;
  serde::DEntTypeUnion type;
  type.Set(std::move(serde_dir));
  serde::DEntT serde_dent;
  serde_dent.type = std::move(type);
  flatbuffers::FlatBufferBuilder fbb(kDefaultFlatBufferBuilderSize, &fb_alloc_);
  fbb.Finish(serde::DEnt::Pack(fbb, &serde_dent));
  return DetachedBuffer(fbb.Release());
}

DetachedBuffer DEntSerde::SerVal(const HardLink& hard_link) {
  serde::HardLinkT serde_hard_link;
  serde_hard_link.parent_id = hard_link.parent_id.val;
  serde_hard_link.name = hard_link.name;
  serde_hard_link.id = hard_link.id.val;
  serde::DEntTypeUnion type;
  type.Set(std::move(serde_hard_link));
  serde::DEntT serde_dent;
  serde_dent.type = std::move(type);
  flatbuffers::FlatBufferBuilder fbb(kDefaultFlatBufferBuilderSize, &fb_alloc_);
  fbb.Finish(serde::DEnt::Pack(fbb, &serde_dent));
  return DetachedBuffer(fbb.Release());
}

std::tuple<InodeID, std::pmr::string> DEntSerde::DeKey(std::string_view key) {
  CHECK_EQ(key.size(), sizeof(InodeID));
  InodeID id;
  static_assert(sizeof(InodeID) == sizeof(int64_t));
  id.val = absl::big_endian::Load64(key.data());
  std::pmr::string name(
      key.data() + sizeof(InodeID), key.size() - sizeof(InodeID), alloc_);
  return std::make_tuple(id, std::move(name));
}

std::variant<Dir, HardLink> DEntSerde::DeVal(std::string_view val) {
  CHECK(flatbuffers::Verifier(reinterpret_cast<const uint8_t*>(val.data()),
                              val.size())
            .VerifyBuffer<serde::DEnt>());
  auto dent = flatbuffers::GetRoot<serde::DEnt>(
      reinterpret_cast<const uint8_t*>(val.data()));
  if (auto dir = dent->type_as_Dir()) {
    return Dir{
        .parent_id = InodeID{dir->parent_id()},
        .name =
            std::pmr::string(dir->name()->c_str(), dir->name()->size(), alloc_),
        .id = InodeID{dir->id()},
    };
  }
  auto hard_link = dent->type_as_HardLink();
  CHECK_NOTNULL(hard_link);
  return HardLink{
      .parent_id = InodeID{hard_link->parent_id()},
      .name = std::pmr::string(
          hard_link->name()->c_str(), hard_link->name()->size(), alloc_),
      .id = InodeID{hard_link->id()},
  };
}

}  // namespace rocketfs
