// Copyright 2025 RocketFS

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

#include "common/logger.h"
#include "common/time_util.h"

// clang-format off
namespace rocketfs { struct ID; }
// clang-format on

namespace rocketfs {

template <typename T>
concept IDGenCompatible =
    std::is_standard_layout_v<T> && sizeof(T) == sizeof(uint64_t) &&
    std::is_constructible_v<T, uint64_t>;

// A UUID generator with time-based ordering:
// 32-bit timestamp
//   Represents the time in seconds, starting from 2025/03/18 00:00:00. It can
//   cover timestamps for the next 100 years.
// 4-bit node ID
//   Uniquely identifies up to 16 meta servers.
// 4-bit clock sequence
//   Used to guarantee uniqueness.
// 24-bit auto-increment ID
//   Supports over 10,000,000 unique IDs per second for each node when `Next` is
//   called.
constexpr auto kEpochTimestampSeconds = 1'742'256'000;
constexpr auto kTimestampBits = 32;
constexpr auto kNodeIDBits = 4;
constexpr auto kClockSequenceBits = 4;
constexpr auto kAutoIncrementIDBits = 24;
constexpr auto kMaxAutoIncrementID = 10'000'000;
struct ID {
  uint32_t timestamp_seconds : kTimestampBits;
  uint8_t node_id : kNodeIDBits;
  uint8_t clock_sequence : kClockSequenceBits;
  uint32_t auto_increment_id : kAutoIncrementIDBits;
};
static_assert(IDGenCompatible<ID>);

template <typename T>
  requires IDGenCompatible<T>
class IDGen {
 public:
  IDGen(uint32_t timestamp_seconds, uint8_t node_id, uint8_t clock_sequence);
  IDGen(const IDGen&) = delete;
  IDGen(IDGen&&) = delete;
  IDGen& operator=(const IDGen&) = delete;
  IDGen& operator=(IDGen&&) = delete;
  ~IDGen() = default;

  T Next();
  void Update(uint32_t timestamp_seconds);

 private:
  const uint8_t node_id_;
  const uint8_t clock_sequence_;
  std::atomic<uint64_t> id_;
};

template <typename T>
  requires IDGenCompatible<T>
IDGen<T>::IDGen(uint32_t timestamp_seconds,
                uint8_t node_id,
                uint8_t clock_sequence)
    : node_id_(node_id), clock_sequence_(clock_sequence) {
  Update(timestamp_seconds);
}

template <typename T>
  requires IDGenCompatible<T>
T IDGen<T>::Next() {
  auto raw_id = id_.fetch_add(1) + 1;
  auto id = *reinterpret_cast<ID*>(&raw_id);
  CHECK_EQ(id.node_id, node_id_);
  CHECK_EQ(id.clock_sequence, clock_sequence_);
  CHECK_LT(id.auto_increment_id, kMaxAutoIncrementID);
  return T(raw_id);
}

template <typename T>
  requires IDGenCompatible<T>
void IDGen<T>::Update(uint32_t timestamp_seconds) {
  auto raw_id = id_.load();
  auto id = *reinterpret_cast<ID*>(&raw_id);
  CHECK_EQ(id.node_id, node_id_);
  CHECK_EQ(id.clock_sequence, clock_sequence_);
  CHECK_LT(id.auto_increment_id, kMaxAutoIncrementID);
  if (id.timestamp_seconds >= timestamp_seconds) {
    return;
  }
  id.timestamp_seconds = timestamp_seconds;
  id.auto_increment_id = 0;
  id_ = *reinterpret_cast<uint64_t*>(&id);
}

class IDGenMgr {
 public:
  explicit IDGenMgr(TimeUtilBase* time_util);
  IDGenMgr(const IDGenMgr&) = delete;
  IDGenMgr(IDGenMgr&&) = delete;
  IDGenMgr& operator=(const IDGenMgr&) = delete;
  IDGenMgr& operator=(IDGenMgr&&) = delete;
  ~IDGenMgr() = default;

  void Start();
  void Stop();

  template <typename T>
    requires IDGenCompatible<T>
  void AddIDGenerator(IDGen<T>* id_generator);

 private:
  TimeUtilBase* time_util_;
  std::unique_ptr<std::thread> thread_;
  std::vector<IDGen<uint64_t>*> id_generators_;
};

template <typename T>
  requires IDGenCompatible<T>
void IDGenMgr::AddIDGenerator(IDGen<T>* id_generator) {
  id_generators_.emplace_back(reinterpret_cast<IDGen<uint64_t>*>(id_generator));
}

}  // namespace rocketfs
