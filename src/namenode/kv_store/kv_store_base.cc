#include "namenode/kv_store/kv_store_base.h"

namespace rocketfs {

ColumnFamilyIndex::ColumnFamilyIndex() : index_(-1) {
}

ColumnFamilyIndex::ColumnFamilyIndex(int32_t index) : index_(index) {
}

int32_t ColumnFamilyIndex::GetIndex() const {
  return index_;
}

const ColumnFamilyIndex kInvalidCFIndex;

}  // namespace rocketfs
