#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <unifex/async_mutex.hpp>
#include <unifex/async_shared_mutex.hpp>
#include <unifex/task.hpp>

#include "common/logger.h"

namespace rocketfs {

template <typename T, typename K>
concept SharedMutexCompatible = requires(K key) {
  { std::hash<K>{}(key) } -> std::convertible_to<std::size_t>;
} && std::equality_comparable<K> && std::constructible_from<T, K>;

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
class SharedMutex {
 public:
  explicit SharedMutex(K key);
  SharedMutex(const SharedMutex&) = delete;
  SharedMutex(SharedMutex&&) = delete;
  SharedMutex& operator=(const SharedMutex&) = delete;
  SharedMutex& operator=(SharedMutex&&) = delete;
  ~SharedMutex();

  void Ref();
  bool Unref();

  K GetKey() const;
  unifex::async_shared_mutex* operator->();

 private:
  T key_;
  int ref_count_;
  unifex::async_shared_mutex mutex_;
};

enum class LockType : int8_t {
  kRead = 1,
  kWrite = 2,
};

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
class SharedMutexBucket;

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
class LockGuard {
 public:
  LockGuard() = default;
  LockGuard(SharedMutexBucket<T, K>* bucket,
            SharedMutex<T, K>* mutex,
            LockType lock_type);
  LockGuard(const LockGuard&) = delete;
  LockGuard(LockGuard&&);
  LockGuard& operator=(const LockGuard&) = delete;
  LockGuard& operator=(LockGuard&&);
  ~LockGuard();

  operator bool() const;
  unifex::task<void> CleanUp() noexcept(false);

 private:
  SharedMutexBucket<T, K>* bucket_{nullptr};
  SharedMutex<T, K>* mutex_{nullptr};
  std::optional<LockType> lock_type_{std::nullopt};
};

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
class SharedMutexBucket {
 public:
  SharedMutexBucket() = default;
  SharedMutexBucket(const SharedMutexBucket&) = delete;
  SharedMutexBucket(SharedMutexBucket&&) = default;
  SharedMutexBucket& operator=(const SharedMutexBucket&) = delete;
  SharedMutexBucket& operator=(SharedMutexBucket&&) = default;
  ~SharedMutexBucket() = default;

  unifex::task<LockGuard<T, K>> AcquireLock(K key, LockType lock_type);

  unifex::task<SharedMutex<T, K>*> GetOrCreate(K key);
  unifex::task<void> Release(SharedMutex<T, K>* mutex);
  unifex::task<std::size_t> Size();

 private:
  unifex::async_mutex mutex_;
  std::unordered_map<K, std::unique_ptr<SharedMutex<T, K>>> mutexes_;
};

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
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
  requires SharedMutexCompatible<T, K>
SharedMutex<T, K>::SharedMutex(K key) : key_(key), ref_count_(0) {
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
SharedMutex<T, K>::~SharedMutex() {
  CHECK_EQ(ref_count_, 0);
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
void SharedMutex<T, K>::Ref() {
  CHECK_GE(ref_count_, 0);
  CHECK_GT(ref_count_ + 1, ref_count_);
  ref_count_++;
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
bool SharedMutex<T, K>::Unref() {
  CHECK_GT(ref_count_, 0);
  ref_count_--;
  return ref_count_ == 0;
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
K SharedMutex<T, K>::GetKey() const {
  return key_;
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
unifex::async_shared_mutex* SharedMutex<T, K>::operator->() {
  return &mutex_;
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
LockGuard<T, K>::LockGuard(SharedMutexBucket<T, K>* bucket,
                           SharedMutex<T, K>* mutex,
                           LockType lock_type)
    : bucket_(bucket), mutex_(mutex), lock_type_(lock_type) {
  CHECK_NOTNULL(bucket_);
  CHECK_NOTNULL(mutex_);
  CHECK(lock_type_ == LockType::kRead || lock_type_ == LockType::kWrite);
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
LockGuard<T, K>::LockGuard(LockGuard&& other) {
  std::swap(bucket_, other.bucket_);
  std::swap(mutex_, other.mutex_);
  std::swap(lock_type_, other.lock_type_);
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
LockGuard<T, K>& LockGuard<T, K>::operator=(LockGuard&& other) {
  if (this != &other) {
    std::swap(bucket_, other.bucket_);
    std::swap(mutex_, other.mutex_);
    std::swap(lock_type_, other.lock_type_);
  }
  return *this;
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
LockGuard<T, K>::~LockGuard() {
  CHECK_NULL(bucket_);
  CHECK_NULL(mutex_);
  CHECK_NULLOPT(lock_type_);
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
LockGuard<T, K>::operator bool() const {
  return lock_type_ != std::nullopt;
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
unifex::task<void> LockGuard<T, K>::CleanUp() noexcept(false) {
  if (lock_type_ == std::nullopt) {
    CHECK_NULL(bucket_);
    CHECK_NULL(mutex_);
    co_return;
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
  co_await bucket_->Release(mutex_);

  bucket_ = nullptr;
  mutex_ = nullptr;
  lock_type_ = std::nullopt;
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
unifex::task<LockGuard<T, K>> SharedMutexBucket<T, K>::AcquireLock(
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
  co_return LockGuard<T, K>(this, mutex, lock_type);
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
unifex::task<SharedMutex<T, K>*> SharedMutexBucket<T, K>::GetOrCreate(K key) {
  SharedMutex<T, K>* mutex = nullptr;
  co_await mutex_.async_lock();
  auto it = mutexes_.find(key);
  if (it == mutexes_.end()) {
    auto mutex = std::make_unique<SharedMutex<T, K>>(key);
    auto key = mutex->GetKey();
    bool inserted = false;
    std::tie(it, inserted) = mutexes_.emplace(key, std::move(mutex));
    CHECK(inserted);
  }
  CHECK_NE(it, mutexes_.end());
  mutex = it->second.get();
  mutex->Ref();
  mutex_.unlock();
  co_return mutex;
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
unifex::task<void> SharedMutexBucket<T, K>::Release(SharedMutex<T, K>* mutex) {
  co_await mutex_.async_lock();
  auto it = mutexes_.find(mutex->GetKey());
  CHECK_NE(it, mutexes_.end());
  CHECK_EQ(it->second.get(), mutex);
  if (mutex->Unref()) {
    mutexes_.erase(it);
  }
  mutex_.unlock();
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
unifex::task<std::size_t> SharedMutexBucket<T, K>::Size() {
  co_await mutex_.async_lock();
  auto size = mutexes_.size();
  mutex_.unlock();
  co_return size;
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
SharedMutexManager<T, K>::SharedMutexManager(int bucket_size)
    : buckets_(bucket_size) {
}

template <typename T, typename K>
  requires SharedMutexCompatible<T, K>
unifex::task<LockGuard<T, K>> SharedMutexManager<T, K>::AcquireLock(
    K key, LockType lock_type) {
  co_return co_await buckets_[std::hash<K>{}(key) % buckets_.size()]
      .AcquireLock(key, lock_type);
}

}  // namespace rocketfs
