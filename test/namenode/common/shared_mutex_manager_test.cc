// Copyright 2025 RocketFS

#include "namenode/common/shared_mutex_manager.h"

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <atomic>
#include <coroutine>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <unifex/coroutine.hpp>
#include <unifex/detail/with_type_erased_tag_invoke.hpp>
#include <unifex/just.hpp>
#include <unifex/just_from.hpp>
#include <unifex/manual_event_loop.hpp>
#include <unifex/overload.hpp>
#include <unifex/scheduler_concepts.hpp>
#include <unifex/sender_for.hpp>
#include <unifex/single_thread_context.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/then.hpp>
#include <unifex/when_all.hpp>

namespace rocketfs {

TEST(SharedMutex, Unref) {
  SharedMutex<std::string, std::string_view> mutex("key");
  mutex.Ref();
  mutex.Ref();
  EXPECT_FALSE(mutex.Unref());
  EXPECT_TRUE(mutex.Unref());
  EXPECT_THROW(mutex.Unref(), std::logic_error);
}

TEST(LockGuardTest, MoveConstrutor) {
  unifex::sync_wait(unifex::just_from([]() -> unifex::task<void> {
    SharedMutexBucket<std::string, std::string_view> bucket;
    auto lock_guard_1 = co_await bucket.AcquireLock("key", LockType::kRead);
    EXPECT_TRUE(lock_guard_1);
    auto lock_guard_2(std::move(lock_guard_1));
    EXPECT_FALSE(lock_guard_1);
    EXPECT_TRUE(lock_guard_2);
    co_await lock_guard_1.CleanUp();
    co_await lock_guard_2.CleanUp();
  }));
}

TEST(LockGuardTest, MoveAssignment) {
  unifex::sync_wait(unifex::just_from([]() -> unifex::task<void> {
    SharedMutexBucket<std::string, std::string_view> bucket;
    auto lock_guard_1 = co_await bucket.AcquireLock("key", LockType::kRead);
    EXPECT_TRUE(lock_guard_1);
    auto lock_guard_2 = co_await bucket.AcquireLock("key", LockType::kRead);
    EXPECT_FALSE(lock_guard_1);
    EXPECT_TRUE(lock_guard_2);
    co_await lock_guard_1.CleanUp();
    co_await lock_guard_2.CleanUp();
  }));
}

TEST(LockGuardTest, DefaultConstructedObjectRequiresNoExplicitCleanup) {
  LockGuard<std::string, std::string_view> lock_guard;
  unifex::sync_wait(lock_guard.CleanUp());
}

TEST(LockGuardTest, CleanUpReleasesMutex) {
  unifex::sync_wait(unifex::just_from([]() -> unifex::task<void> {
    SharedMutexBucket<std::string, std::string_view> bucket;
    auto lock_guard = co_await bucket.AcquireLock("key", LockType::kRead);
    co_await lock_guard.CleanUp();
  }));
}

TEST(SharedMutextBucketTest, SameKeyLocksSameMutex) {
  unifex::sync_wait(unifex::just_from([]() -> unifex::task<void> {
    SharedMutexBucket<std::string, std::string_view> bucket;
    auto lock_guard_1 = co_await bucket.AcquireLock("key", LockType::kRead);
    EXPECT_EQ(co_await bucket.Size(), 1);
    auto lock_guard_2 = co_await bucket.AcquireLock("key", LockType::kRead);
    EXPECT_EQ(co_await bucket.Size(), 1);
    co_await lock_guard_1.CleanUp();
    EXPECT_EQ(co_await bucket.Size(), 1);
    co_await lock_guard_2.CleanUp();
    EXPECT_EQ(co_await bucket.Size(), 0);
  }));
}

TEST(SharedMutextBucketTest, DifferentKeysLockDifferentMutexes) {
  unifex::sync_wait(unifex::just_from([]() -> unifex::task<void> {
    SharedMutexBucket<std::string, std::string_view> bucket;
    auto lock_guard_1 = co_await bucket.AcquireLock("key-1", LockType::kRead);
    EXPECT_EQ(co_await bucket.Size(), 1);
    auto lock_guard_2 = co_await bucket.AcquireLock("key-2", LockType::kRead);
    EXPECT_EQ(co_await bucket.Size(), 2);
    co_await lock_guard_1.CleanUp();
    EXPECT_EQ(co_await bucket.Size(), 1);
    co_await lock_guard_2.CleanUp();
    EXPECT_EQ(co_await bucket.Size(), 0);
  }));
}

TEST(SharedMutexBucketTest, AcquireLock) {
  SharedMutexBucket<std::string, std::string_view> bucket;
  constexpr int iterations = 1;

  int unique_state = 0;
  auto make_unique_task = [&](unifex::manual_event_loop::scheduler scheduler)
      -> unifex::task<void> {
    for (int i = 0; i < iterations; i++) {
      auto lock_guard = co_await bucket.AcquireLock("key", LockType::kWrite);
      co_await unifex::schedule(scheduler);
      unique_state += 1;
      co_await lock_guard.CleanUp();
    }
  };

  std::atomic<int> stolen_unique_state = 0;
  std::atomic<int> shared_state = 0;
  auto make_shared_task = [&](unifex::manual_event_loop::scheduler scheduler)
      -> unifex::task<void> {
    for (int i = 0; i < iterations; i++) {
      auto lock_guard = co_await bucket.AcquireLock("key", LockType::kRead);
      co_await unifex::schedule(scheduler);
      int expected = 0;
      if (unique_state != 0 &&
          stolen_unique_state.compare_exchange_strong(expected, unique_state)) {
        unique_state = 0;
        co_await unifex::schedule(scheduler);
        unique_state = stolen_unique_state.exchange(0);
      }
      shared_state++;
      co_await lock_guard.CleanUp();
    }
  };

  unifex::single_thread_context ctx1;
  unifex::single_thread_context ctx2;
  unifex::single_thread_context ctx3;
  unifex::single_thread_context ctx4;
  unifex::sync_wait(unifex::when_all(make_unique_task(ctx1.get_scheduler()),
                                     make_unique_task(ctx2.get_scheduler()),
                                     make_shared_task(ctx3.get_scheduler()),
                                     make_shared_task(ctx4.get_scheduler())));

  EXPECT_EQ(unifex::sync_wait(bucket.Size()), 0);
  EXPECT_EQ(2 * iterations, unique_state);
  EXPECT_EQ(2 * iterations, shared_state);
}

TEST(SharedMutexBucketTest, MutexAddressesRemainStable) {
  constexpr int iterations = 1024;
  unifex::sync_wait(unifex::just_from([]() -> unifex::task<void> {
    SharedMutexBucket<std::string, std::string_view> bucket;
    std::vector<SharedMutex<std::string, std::string_view>*> mutexes;
    for (int i = 0; i < iterations; i++) {
      auto mutex = co_await bucket.GetOrCreate(fmt::format("key-{}", i));
      mutexes.push_back(mutex);
    }
    for (int i = 0; i < iterations; i++) {
      auto mutex = co_await bucket.GetOrCreate(fmt::format("key-{}", i));
      EXPECT_EQ(mutex, mutexes.at(i));
      co_await bucket.Release(mutex);
    }
    for (int i = 0; i < iterations; i++) {
      co_await bucket.Release(mutexes[i]);
    }
  }));
}

TEST(SharedMutexManagerTest, AcquireLock) {
  SharedMutexManager<std::string, std::string_view> manager(1);
  constexpr int iterations = 100'000;

  int unique_state = 0;
  auto make_unique_task = [&](unifex::manual_event_loop::scheduler scheduler)
      -> unifex::task<void> {
    for (int i = 0; i < iterations; i++) {
      auto lock_guard = co_await manager.AcquireLock("key", LockType::kWrite);
      co_await unifex::schedule(scheduler);
      unique_state += 1;
      co_await lock_guard.CleanUp();
    }
  };

  std::atomic<int> stolen_unique_state = 0;
  std::atomic<int> shared_state = 0;
  auto make_shared_task = [&](unifex::manual_event_loop::scheduler scheduler)
      -> unifex::task<void> {
    for (int i = 0; i < iterations; i++) {
      auto lock_guard = co_await manager.AcquireLock("key", LockType::kRead);
      co_await unifex::schedule(scheduler);
      int expected = 0;
      if (unique_state != 0 &&
          stolen_unique_state.compare_exchange_strong(expected, unique_state)) {
        unique_state = 0;
        co_await unifex::schedule(scheduler);
        unique_state = stolen_unique_state.exchange(0);
      }
      shared_state++;
      co_await lock_guard.CleanUp();
    }
  };

  unifex::single_thread_context ctx1;
  unifex::single_thread_context ctx2;
  unifex::single_thread_context ctx3;
  unifex::single_thread_context ctx4;
  unifex::sync_wait(unifex::when_all(make_unique_task(ctx1.get_scheduler()),
                                     make_unique_task(ctx2.get_scheduler()),
                                     make_shared_task(ctx3.get_scheduler()),
                                     make_shared_task(ctx4.get_scheduler())));

  EXPECT_EQ(2 * iterations, unique_state);
  EXPECT_EQ(2 * iterations, shared_state);
}

}  // namespace rocketfs
