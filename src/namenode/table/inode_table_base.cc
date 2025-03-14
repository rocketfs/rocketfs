// Copyright 2025 RocketFS

#include "namenode/table/inode_table_base.h"

#include <flatbuffers/buffer.h>
#include <flatbuffers/verifier.h>

#include <span>
#include <string>
#include <utility>

#include "common/logger.h"

namespace rocketfs {

Inode::Inode(std::pmr::string basic_info_str, std::pmr::string timestamps_str)
    : basic_info_str_(std::move(basic_info_str)),
      timestamps_str_(std::move(timestamps_str)) {
  std::span<const uint8_t> basic_info_span(
      reinterpret_cast<uint8_t*>(basic_info_str_.data()),
      basic_info_str_.size());
  CHECK(flatbuffers::Verifier(basic_info_span.data(), basic_info_span.size())
            .VerifyBuffer<serde::InodeBasicInfo>());
  basic_info_ =
      flatbuffers::GetRoot<serde::InodeBasicInfo>(basic_info_span.data());
  CHECK_NOTNULL(basic_info_);

  std::span<const uint8_t> timestamps_span(
      reinterpret_cast<uint8_t*>(timestamps_str_.data()),
      timestamps_str_.size());
  CHECK(flatbuffers::Verifier(timestamps_span.data(), timestamps_span.size())
            .VerifyBuffer<serde::InodeTimestamps>());
  timestamps_ =
      flatbuffers::GetRoot<serde::InodeTimestamps>(timestamps_span.data());
  CHECK_NOTNULL(timestamps_);
}

InodeID Inode::inode_id() const {
  return {basic_info_->inode_id()};
}

serde::InodeType Inode::inode_type() const {
  return basic_info_->inode_type_type();
}

const serde::File* Inode::as_file() const {
  return basic_info_->inode_type_as_File();
}

const serde::Directory* Inode::as_directory() const {
  return basic_info_->inode_type_as_Directory();
}

const serde::Symlink* Inode::as_symlink() const {
  return basic_info_->inode_type_as_Symlink();
}

int64_t Inode::created_txid() const {
  return basic_info_->created_txid();
}

int64_t Inode::renamed_txid() const {
  return basic_info_->renamed_txid();
}

int64_t Inode::ctime_in_nanoseconds() const {
  return timestamps_->ctime_in_nanoseconds();
}

int64_t Inode::mtime_in_nanoseconds() const {
  return timestamps_->mtime_in_nanoseconds();
}

int64_t Inode::atime_in_nanoseconds() const {
  return timestamps_->atime_in_nanoseconds();
}

const serde::InodeBasicInfo& Inode::basic_info() const {
  return *basic_info_;
}

const serde::InodeTimestamps& Inode::timestamps() const {
  return *timestamps_;
}

InodeT::InodeT(const Inode& inode) {
  inode.basic_info_->UnPackTo(&basic_info_);
  inode.timestamps_->UnPackTo(&timestamps_);
}

InodeID& InodeT::inode_id() {
  static_assert(sizeof(InodeID) == sizeof(basic_info_.inode_id));
  return reinterpret_cast<InodeID&>(basic_info_.inode_id);
}

serde::InodeTypeUnion& InodeT::inode_type_union() {
  return basic_info_.inode_type;
}

int64_t& InodeT::created_txid() {
  return basic_info_.created_txid;
}

int64_t& InodeT::renamed_txid() {
  return basic_info_.renamed_txid;
}

int64_t& InodeT::ctime_in_nanoseconds() {
  return timestamps_.ctime_in_nanoseconds;
}

int64_t& InodeT::mtime_in_nanoseconds() {
  return timestamps_.mtime_in_nanoseconds;
}

int64_t& InodeT::atime_in_nanoseconds() {
  return timestamps_.atime_in_nanoseconds;
}

InodeID InodeT::inode_id() const {
  return {basic_info_.inode_id};
}

serde::InodeType InodeT::inode_type() const {
  return basic_info_.inode_type.type;
}

const serde::FileT* InodeT::as_file() const {
  return basic_info_.inode_type.AsFile();
}

const serde::DirectoryT* InodeT::as_directory() const {
  return basic_info_.inode_type.AsDirectory();
}

const serde::SymlinkT* InodeT::as_symlink() const {
  return basic_info_.inode_type.AsSymlink();
}

int64_t InodeT::created_txid() const {
  return basic_info_.created_txid;
}

int64_t InodeT::renamed_txid() const {
  return basic_info_.renamed_txid;
}

int64_t InodeT::ctime_in_nanoseconds() const {
  return timestamps_.ctime_in_nanoseconds;
}

int64_t InodeT::mtime_in_nanoseconds() const {
  return timestamps_.mtime_in_nanoseconds;
}

int64_t InodeT::atime_in_nanoseconds() const {
  return timestamps_.atime_in_nanoseconds;
}

const serde::InodeBasicInfoT& InodeT::basic_info() const {
  return basic_info_;
}

const serde::InodeTimestampsT& InodeT::timestamps() const {
  return timestamps_;
}

}  // namespace rocketfs
