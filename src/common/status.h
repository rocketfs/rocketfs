#include <string>
#include <string_view>

namespace rocketfs {

enum class StatusCode {
  kOK = 0,

  // Error encountered in `KVStoreBase`.
  kKVStoreKeyNotFoundError = 1001,
};

class Status {
 private:
  Status(StatusCode status_code, std::string_view msg);

 public:
  Status(const Status&) = default;
  Status(Status&&) = default;
  Status& operator=(const Status&) = default;
  Status& operator=(Status&&) = default;
  ~Status() = default;

  bool IsOK() const;
  bool IsKVStoreKeyNotFoundError() const;

  std::string_view GetMsg() const;

 public:
  static Status OK(std::string_view msg = "");
  static Status KVStoreKeyNotFoundError(
      const Status& internal_error = Status::OK(), std::string_view msg = "");

 private:
  StatusCode status_code_;
  std::string msg_;
};

}  // namespace rocketfs
