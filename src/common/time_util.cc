// Copyright 2025 RocketFS

#include "common/time_util.h"

#include <chrono>
#include <cstdint>

namespace rocketfs {

int64_t TimeUtil::GetCurrentTimeInNanoseconds() const {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

}  // namespace rocketfs
