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

void SharedMutex::Lock() {
  mutex_.lock();
}

void SharedMutex::Unlock() {
  mutex_.unlock();
}

void SharedMutex::LockShared() {
  mutex_.lock_shared();
}

void SharedMutex::UnlockShared() {
  mutex_.unlock_shared();
}




}  // namespace rocketfs
