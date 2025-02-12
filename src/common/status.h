#pragma once

#include <fmt/core.h>

#include <string>
#include <string_view>

namespace rocketfs {

enum class StatusCode {
  kOK = 0,
  kInvalidArgumentError = 1,

  // Error encountered in `KVStoreBase`.
  kNotFoundError = 1001,
};

class Status {
 private:
  inline Status(StatusCode status_code, std::string_view msg);

 public:
  inline static Status OK(std::string_view msg = "");
  inline static Status InvalidArgumentError(
      const Status& internal_error = Status::OK(), std::string_view msg = "");
  inline static Status NotFoundError(
      const Status& internal_error = Status::OK(), std::string_view msg = "");

  Status(const Status&) = default;
  Status(Status&&) = default;
  Status& operator=(const Status&) = default;
  Status& operator=(Status&&) = default;
  ~Status() = default;

  inline bool IsOK() const;
  inline bool IsInvalidArgumentError() const;
  inline bool IsNotFoundError() const;

  inline std::string_view GetMsg() const;

 private:
  StatusCode status_code_;
  std::string msg_;
};

Status Status::OK(std::string_view msg) {
  return Status(StatusCode::kOK, msg);
}

Status Status::InvalidArgumentError(const Status& internal_error,
                                    std::string_view msg) {
  return Status(
      StatusCode::kInvalidArgumentError,
      fmt::format("not found error with internal error: [{}] and msg: {}",
                  internal_error.GetMsg(),
                  msg));
}

Status Status::NotFoundError(const Status& internal_error,
                             std::string_view msg) {
  return Status(
      StatusCode::kNotFoundError,
      fmt::format("not found error with internal error: [{}] and msg: {}",
                  internal_error.GetMsg(),
                  msg));
}

std::string_view Status::GetMsg() const {
  return msg_;
}

Status::Status(StatusCode status_code, std::string_view msg)
    : status_code_(status_code), msg_(msg) {
}

bool Status::IsOK() const {
  return status_code_ == StatusCode::kOK;
}

bool Status::IsNotFoundError() const {
  return status_code_ == StatusCode::kNotFoundError;
}

}  // namespace rocketfs
