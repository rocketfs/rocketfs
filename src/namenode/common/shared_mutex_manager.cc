#include "namenode/common/shared_mutex_manager.h"

#include "common/logger.h"

namespace rocketfs {

SharedMutex::SharedMutex() : ref_count_(0) {
}

void SharedMutex::Ref() {
  CHECK_GE(ref_count_, 0);
  CHECK_GT(ref_count_ + 1, ref_count_);
  ref_count_++;
}

bool SharedMutex::Unref() {
  CHECK_GT(ref_count_, 0);
  ref_count_--;
  return ref_count_ == 0;
}

unifex::async_shared_mutex* SharedMutex::operator->() {
  return &mutex_;
}

}  // namespace rocketfs
