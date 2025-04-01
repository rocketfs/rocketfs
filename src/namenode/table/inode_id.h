// Copyright 2025 RocketFS

#pragma once

#include <cstdint>
#include <limits>

#include "namenode/common/id_gen.h"

namespace rocketfs {

struct InodeID {
  uint64_t val{std::numeric_limits<uint64_t>::max()};
};

inline bool operator==(const InodeID& lhs, const InodeID& rhs) {
  return lhs.val == rhs.val;
}

inline bool operator!=(const InodeID& lhs, const InodeID& rhs) {
  return !(lhs == rhs);
}

constexpr InodeID kInvalidInodeID{std::numeric_limits<uint64_t>::max()};
constexpr InodeID kRootInodeID{1};

using InodeIDGen = IDGen<InodeID>;

}  // namespace rocketfs
