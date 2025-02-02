#pragma once

#include <cstdint>

namespace rocketfs {

struct ColumnFamilyIndex {
  int8_t index{-1};
};

inline bool operator==(const ColumnFamilyIndex& lhs,
                       const ColumnFamilyIndex& rhs) {
  return lhs.index == rhs.index;
}

inline bool operator!=(const ColumnFamilyIndex& lhs,
                       const ColumnFamilyIndex& rhs) {
  return !(lhs == rhs);
}

constexpr ColumnFamilyIndex kInvalidCFIndex{-1};
constexpr ColumnFamilyIndex kINodeCFIndex{0};

}  // namespace rocketfs
