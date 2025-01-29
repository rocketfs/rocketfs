// Copyright 2025 RocketFS
#pragma once

#include <quill/Backend.h>  // IWYU pragma: keep
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/core/ThreadContextManager.h>

#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

namespace rocketfs {

extern quill::Logger* logger;

inline void Check(
    bool condition,
    std::string_view str,
    std::source_location location = std::source_location::current()) {
  if (!condition) {
    LOG_CRITICAL(logger, "Check failed: {}.", str);
    throw std::logic_error("Check failed: " + std::string(str) + ".");
  }
}

}  // namespace rocketfs

#define CHECK(condition) rocketfs::Check((condition), #condition)
#define CHECK_NULL(ptr) CHECK((ptr) == nullptr)
#define CHECK_NOTNULL(ptr) CHECK((ptr) != nullptr)
#define CHECK_NULLOPT(opt) CHECK((opt) == std::nullopt)
#define CHECK_NE(lhs, rhs) CHECK((lhs) != (rhs))
#define CHECK_EQ(lhs, rhs) CHECK((lhs) == (rhs))
#define CHECK_LT(lhs, rhs) CHECK((lhs) < (rhs))
#define CHECK_GT(lhs, rhs) CHECK((lhs) > (rhs))
#define CHECK_GE(lhs, rhs) CHECK((lhs) >= (rhs))
