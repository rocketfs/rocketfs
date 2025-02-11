#pragma once

#include <cstdint>
#include <memory>
#include <unifex/async_shared_mutex.hpp>
#include <unordered_map>

namespace rocketfs {

class SharedMutex {
 public:
  SharedMutex();
  SharedMutex(const SharedMutex&) = delete;
  SharedMutex(SharedMutex&&) = delete;
  SharedMutex& operator=(const SharedMutex&) = delete;
  SharedMutex& operator=(SharedMutex&&) = delete;
  ~SharedMutex() = default;

  void Ref();
  bool Unref();

  void Lock();
  void Unlock();
  void LockShared();
  void UnlockShared();

 private:
  int ref_count_;
  unifex::async_shared_mutex mutex_;
};

enum class LockType : int8_t {
  kRead = 1,
  kWrite = 2,
};

template <typename T, typename K>
class LockGuard {
 public:
  LockGuard(K key,
            SharedMutexBucket<T>* bucket,
            SharedMutex* mutex,
            LockType lock_type);
  LockGuard(const LockGuard&) = delete;
  LockGuard(LockGuard&&) = delete;
  LockGuard& operator=(const LockGuard&) = delete;
  LockGuard& operator=(LockGuard&&) = delete;
  ~LockGuard();

 private:
  K key_;
  SharedMutexBucket<T>* bucket_;
  SharedMutex* mutex_;
  LockType lock_type_;
};

template <typename T, typename K>
  requires std::hashable<T> && std::equality_comparable<T> &&
           std::constructible_from<T, K>
class SharedMutexBucket {
 public:
  SharedMutexBucket() = default;
  SharedMutexBucket(const SharedMutexBucket&) = delete;
  SharedMutexBucket(SharedMutexBucket&&) = default;
  SharedMutexBucket& operator=(const SharedMutexBucket&) = delete;
  SharedMutexBucket& operator=(SharedMutexBucket&&) = default;
  ~SharedMutexBucket() = default;

  LockGuard<T> Lock(K key, LockType lock_type);

  SharedMutex* GetOrCreate(K key);
  void Release(K key, SharedMutex* mutex);

 private:
  std::unordered_map<K, unifex::async_mutex> mutexes_;
};

template <typename T, typename K>
  requires std::hashable<T> && std::equality_comparable<T> &&
           std::constructible_from<T, K>
class SharedMutexManager {
 public:
  explicit SharedMutexManager(int bucket_size);
  SharedMutexManager(const SharedMutexManager&) = delete;
  SharedMutexManager(SharedMutexManager&&) = delete;
  SharedMutexManager& operator=(const SharedMutexManager&) = delete;
  SharedMutexManager& operator=(SharedMutexManager&&) = delete;
  ~SharedMutexManager() = default;

  std::shared_ptr<LockGuard<T, K>> AcquireLock(K key, LockType lock_type);

 private:
  std::vector<SharedMutexBucket<T>> buckets_;
};

template <typename T, typename K>
LockGuard<T, K>::LockGuard(K key,
                           SharedMutexBucket<T>* bucket,
                           SharedMutex* mutex,
                           LockType lock_type)
    : bucket_(bucket), mutex_(mutex), lock_type_(lock_type) {
  CHECK_NOTNULL(bucket_);
  CHECK_NOTNULL(mutex_);
  CHECK(lock_type_ == LockType::kRead || lock_type_ == LockType::kWrite);
  switch (lock_type_) {
    case LockType::kRead: {
      mutex_->LockShared();
    } break;
    case LockType::kWrite: {
      mutex_->Lock();
    } break;
  }
}

template <typename T, typename K>
LockGuard<T, K>::~LockGuard() {
  CHECK(lock_type_ == LockType::kRead || lock_type_ == LockType::kWrite);
  switch (lock_type_) {
    case LockType::kRead: {
      mutex_->UnlockShared();
    } break;
    case LockType::kWrite: {
      mutex_->Unlock();
    } break;
  }
  bucket_->Release(key_, mutex_);
}

}  // namespace rocketfs
