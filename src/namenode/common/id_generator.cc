// Copyright 2025 RocketFS

#include "namenode/common/id_generator.h"

#include <chrono>
#include <memory>

#include "common/time_util.h"

namespace rocketfs {

IDGeneratorManager::IDGeneratorManager(TimeUtilBase* time_util)
    : time_util_(time_util) {
  CHECK_NOTNULL(time_util_);
}

void IDGeneratorManager::Start() {
  CHECK_NULL(thread_);
  thread_ = std::make_unique<std::thread>([this]() {
    while (true) {
      for (auto id_generator : id_generators_) {
        auto current_timestamp_seconds =
            time_util_->GetCurrentTimeInNanoseconds() /
            (kMillisecondToMicrosecond * kMicrosecondToNanosecond);
        CHECK_GT(current_timestamp_seconds, kEpochTimestampSeconds);
        current_timestamp_seconds -= kEpochTimestampSeconds;
        id_generator->Update(current_timestamp_seconds);
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });
}

void IDGeneratorManager::Stop() {
  CHECK_NOTNULL(thread_);
  thread_->join();
  thread_ = nullptr;
  id_generators_.clear();
}

}  // namespace rocketfs
