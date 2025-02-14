// Copyright 2025 RocketFS

#include "namenode/common/shared_mutex_manager.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <unifex/detail/with_type_erased_tag_invoke.hpp>
#include <unifex/overload.hpp>
#include <unifex/sender_for.hpp>
#include <utility>

namespace rocketfs {

TEST(LockGuardTest, DefaultConstructedObjectHasTrivialDestructor) {
  LockGuard<std::string, std::string_view> lock_guard;
}

TEST(SharedMutexManagerTest, Basic) {
  SharedMutexManager<std::string, std::string_view> manager(1);
}

}  // namespace rocketfs
