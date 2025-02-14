// Copyright 2025 RocketFS

#include "namenode/common/shared_mutex_manager.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace rocketfs {

TEST(SharedMutexManagerTest, Basic) {
  SharedMutexManager<std::string, std::string_view> manager(1);
}

}  // namespace rocketfs
