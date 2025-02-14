#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <unifex/async_mutex.hpp>
#include <unifex/async_shared_mutex.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/task.hpp>
#include <unordered_map>
#include <vector>

#include "common/logger.h"

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

  unifex::async_shared_mutex* operator->();

 private:
  int ref_count_;
  unifex::async_shared_mutex mutex_;
};

enum class LockType : int8_t {
  kRead = 1,
  kWrite = 2,
};

template <typename T, typename K>
  requires std::equality_comparable<T> && std::constructible_from<T, K>
class SharedMutexBucket;

template <typename T, typename K>
class LockGuard {
 public:
  LockGuard() = default;
  LockGuard(K key,
            SharedMutexBucket<T, K>* bucket,
            SharedMutex* mutex,
            LockType lock_type);
  LockGuard(const LockGuard&) = delete;
  LockGuard(LockGuard&&) = default;
  LockGuard& operator=(const LockGuard&) = delete;
  LockGuard& operator=(LockGuard&&) = default;
  ~LockGuard();

 private:
  K key_;
  SharedMutexBucket<T, K>* bucket_;
  SharedMutex* mutex_;
  std::optional<LockType> lock_type_;
};

template <typename T, typename K>
  requires std::equality_comparable<T> && std::constructible_from<T, K>
class SharedMutexBucket {
 public:
  SharedMutexBucket() = default;
  SharedMutexBucket(const SharedMutexBucket&) = delete;
  SharedMutexBucket(SharedMutexBucket&&) = default;
  SharedMutexBucket& operator=(const SharedMutexBucket&) = delete;
  SharedMutexBucket& operator=(SharedMutexBucket&&) = default;
  ~SharedMutexBucket() = default;

  unifex::task<LockGuard<T, K>> Lock(K key, LockType lock_type);

  unifex::task<SharedMutex*> GetOrCreate(K key);
  unifex::task<void> Release(K key, SharedMutex* mutex);

 private:
  unifex::async_mutex mutex_;
  std::unordered_map<K, SharedMutex> mutexes_;
};

template <typename T, typename K>
  requires std::equality_comparable<T> && std::constructible_from<T, K>
class SharedMutexManager {
 public:
  explicit SharedMutexManager(int bucket_size);
  SharedMutexManager(const SharedMutexManager&) = delete;
  SharedMutexManager(SharedMutexManager&&) = delete;
  SharedMutexManager& operator=(const SharedMutexManager&) = delete;
  SharedMutexManager& operator=(SharedMutexManager&&) = delete;
  ~SharedMutexManager() = default;

  unifex::task<LockGuard<T, K>> AcquireLock(K key, LockType lock_type);

 private:
  std::vector<SharedMutexBucket<T, K>> buckets_;
};

template <typename T, typename K>
LockGuard<T, K>::LockGuard(K key,
                           SharedMutexBucket<T, K>* bucket,
                           SharedMutex* mutex,
                           LockType lock_type)
    : bucket_(bucket), mutex_(mutex), lock_type_(lock_type) {
  CHECK_NOTNULL(bucket_);
  CHECK_NOTNULL(mutex_);
  CHECK(lock_type_ == LockType::kRead || lock_type_ == LockType::kWrite);
}

template <typename T, typename K>
LockGuard<T, K>::~LockGuard() {
  if (lock_type_ == std::nullopt) {
    CHECK_NULL(bucket_);
    CHECK_NULL(mutex_);
    return;
  }

  CHECK(*lock_type_ == LockType::kRead || *lock_type_ == LockType::kWrite);
  switch (*lock_type_) {
    case LockType::kRead: {
      (*mutex_)->unlock_shared();
    } break;
    case LockType::kWrite: {
      (*mutex_)->unlock();
    } break;
  }
  unifex::sync_wait(bucket_->Release(key_, mutex_));
}

template <typename T, typename K>
  requires std::equality_comparable<T> && std::constructible_from<T, K>
unifex::task<LockGuard<T, K>> SharedMutexBucket<T, K>::Lock(
    K key, LockType lock_type) {
  auto* mutex = co_await GetOrCreate(key);
  CHECK(lock_type == LockType::kRead || lock_type == LockType::kWrite);
  switch (lock_type) {
    case LockType::kRead: {
      co_await (*mutex)->async_lock_shared();
    } break;
    case LockType::kWrite: {
      co_await (*mutex)->async_lock();
    } break;
  }
  co_return LockGuard<T, K>(key, this, mutex, lock_type);
}

template <typename T, typename K>
  requires std::equality_comparable<T> && std::constructible_from<T, K>
unifex::task<SharedMutex*> SharedMutexBucket<T, K>::GetOrCreate(K key) {
  SharedMutex* mutex = nullptr;
  co_await mutex_.async_lock();
  mutex = &mutexes_[key];
  mutex->Ref();
  mutex_.unlock();
  co_return mutex;
}

template <typename T, typename K>
  requires std::equality_comparable<T> && std::constructible_from<T, K>
unifex::task<void> SharedMutexBucket<T, K>::Release(K key, SharedMutex* mutex) {
  co_await mutex_.async_lock();
  auto it = mutexes_.find(key);
  CHECK_NE(it, mutexes_.end());
  CHECK_EQ(&it->second, mutex);
  if (mutex->Unref()) {
    mutexes_.erase(it);
  }
  mutex_.unlock();
}

template <typename T, typename K>
  requires std::equality_comparable<T> && std::constructible_from<T, K>
SharedMutexManager<T, K>::SharedMutexManager(int bucket_size)
    : buckets_(bucket_size) {
}

template <typename T, typename K>
  requires std::equality_comparable<T> && std::constructible_from<T, K>
unifex::task<LockGuard<T, K>> SharedMutexManager<T, K>::AcquireLock(
    K key, LockType lock_type) {
  co_return co_await buckets_[std::hash<K>{}(key) % buckets_.size()].Lock(
      key, lock_type);
}

}  // namespace rocketfs
