// Copyright 2025 RocketFS

#pragma once

#include <cstdint>

namespace rocketfs {

constexpr int64_t kSecToMs = 1000;
constexpr int64_t kMsToUs = 1000;
constexpr int64_t kUsToNs = 1000;
constexpr int64_t kSecToNs = kSecToMs * kMsToUs * kUsToNs;

class TimeUtilBase {
 public:
  TimeUtilBase() = default;
  TimeUtilBase(const TimeUtilBase&) = delete;
  TimeUtilBase(TimeUtilBase&&) = delete;
  TimeUtilBase& operator=(const TimeUtilBase&) = delete;
  TimeUtilBase& operator=(TimeUtilBase&&) = delete;
  virtual ~TimeUtilBase() = default;

  virtual int64_t NowNs() const = 0;
};

class TimeUtil : public TimeUtilBase {
 public:
  TimeUtil() = default;
  TimeUtil(const TimeUtil&) = delete;
  TimeUtil(TimeUtil&&) = delete;
  TimeUtil& operator=(const TimeUtil&) = delete;
  TimeUtil& operator=(TimeUtil&&) = delete;
  ~TimeUtil() override = default;

  int64_t NowNs() const override;
};

}  // namespace rocketfs
