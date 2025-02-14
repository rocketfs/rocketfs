// Copyright 2025 RocketFS

#include <quill/LogMacros.h>
#include <quill/Logger.h>

namespace rocketfs {

extern quill::Logger* logger;

}  // namespace rocketfs

#define CHECK(condition)                                    \
  do {                                                      \
    if (!(condition)) {                                     \
      LOG_CRITICAL(logger, "Check failed: {}." #condition); \
    }                                                       \
  } while (0)

#define CHECK_NULL(ptr) CHECK((ptr) == nullptr)
#define CHECK_NOTNULL(ptr) CHECK((ptr) != nullptr)
#define CHECK_NE(lhs, rhs) CHECK((lhs) != (rhs))
#define CHECK_EQ(lhs, rhs) CHECK((lhs) == (rhs))
#define CHECK_LT(lhs, rhs) CHECK((lhs) < (rhs))
#define CHECK_GT(lhs, rhs) CHECK((lhs) > (rhs))
#define CHECK_GE(lhs, rhs) CHECK((lhs) >= (rhs))
