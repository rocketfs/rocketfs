#pragma once

#include <cstdint>
#include <string_view>

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
constexpr ColumnFamilyIndex kDefaultCFIndex{0};
constexpr ColumnFamilyIndex kInodeBasicInfoCFIndex{1};
constexpr ColumnFamilyIndex kInodeTimestampsCFIndex{2};

}  // namespace rocketfs
