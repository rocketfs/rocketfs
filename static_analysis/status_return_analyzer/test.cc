// Copyright 2025 RocketFS

namespace rocketfs {

class Status {
 public:
  static Status OK();
  static Status InvalidParameterException();
  static Status IOException();
  static Status LogicalError();
  static Status InternalError();

  bool IsOK() const;
  bool IsInvalidParameterException() const;
  bool IsIOException() const;
  bool IsLogicalError() const;
  bool IsInternalError() const;
};

class StatusReturnAnalyzerTestSuite {
  /**
   * @retval Status::OK()
   * @retval Status::InvalidParameterException()
   * @retval Status::IOException()
   * @retval Status::LogicalError()
   */
  Status TestRetvalCommentsConsistentWithReturnValues() {
    if (false) {
      return Status::InvalidParameterException();
    }
    if (false) {
      return Status::IOException();
    }
    if (false) {
      return Status::LogicalError();
    }
    return Status::OK();
  }

  /**
   * @retval Status::OK()
   * @retval Status::LogicalError()
   */
  Status TestRetvalCommentsWithMoreValuesThanReturnValues() {
    return Status::OK();
  }

  /**
   * @retval Status::OK()
   */
  Status TestRetvalCommentsWithFewerValuesThanReturnValues() {
    if (false) {
      return Status::LogicalError();
    }
    return Status::OK();
  }

  /**
   * @retval Status::OK()
   * @retval Status::InvalidParameterException()
   * @retval Status::IOException()
   * @retval Status::LogicalError()
   */
  Status TestDirectReturnFunctionCallResult() {
    return TestRetvalCommentsConsistentWithReturnValues();
  }

  /**
   * @retval Status::OK()
   * @retval Status::InvalidParameterException()
   * @retval Status::IOException()
   * @retval Status::LogicalError()
   */
  Status TestReturnConstVariableHoldingFunctionCallResult() {
    const auto s = TestRetvalCommentsConsistentWithReturnValues();
    return s;
  }

  /**
   * @retval Status::OK()
   * @retval Status::InvalidParameterException()
   * @retval Status::IOException()
   * @retval Status::LogicalError()
   * @retval Status::InternalError()
   */
  Status TestReturnConstVariableFromNestedBlock() {
    const auto s = TestRetvalCommentsConsistentWithReturnValues();
    for (; false;) {
      if (false) {
        while (true) {
          return s;
        }
      }
    }
    return Status::InternalError();
  }

  /**
   * @retval Status::OK()
   * @retval Status::LogicalError()
   * @retval Status::InternalError()
   */
  Status TestReturnConvertedErrorOnSpecificErrors() {
    const auto s = TestRetvalCommentsConsistentWithReturnValues();
    if (s.IsInvalidParameterException() || s.IsIOException()) {
      return Status::InternalError();
    }
    return s;
  }

  /**
   * @retval Status::OK()
   * @retval Status::IOException()
   * @retval Status::LogicalError()
   */
  Status TestPropagateSpecificErrors() {
    const auto s = TestRetvalCommentsConsistentWithReturnValues();
    if (s.IsIOException() || s.IsLogicalError() || s.IsInternalError()) {
      return s;
    }
    return Status::OK();
  }

  /**
   * @retval Status::OK()
   * @retval Status::InternalError()
   */
  Status TestReturnConvertedErrorOnAnyError() {
    const auto s = TestRetvalCommentsConsistentWithReturnValues();
    if (!s.IsOK()) {
      return Status::InternalError();
    }
    return s;
  }

  /**
   * @retval Status::InvalidParameterException()
   * @retval Status::IOException()
   * @retval Status::LogicalError()
   * @retval Status::InternalError()
   */
  Status TestPropagateAnyError() {
    const auto s = TestRetvalCommentsConsistentWithReturnValues();
    if (!s.IsOK()) {
      return s;
    }
    return Status::InternalError();
  }

  /**
   * @retval Status::OK()
   * @retval Status::InvalidParameterException()
   * @retval Status::IOException()
   * @retval Status::LogicalError()
   * @retval Status::InternalError()
   *
   * @note
   * Compared to `TestReturnConvertedErrorOnSpecificErrors`, the value
   * optimization for the variable `s` is disabled.
   */
  Status TestDisableOptimizationIfVarDeclAndIfStmtAreInDifferentScopes() {
    const auto s = TestRetvalCommentsConsistentWithReturnValues();
    if (false) {
      if (s.IsInvalidParameterException() || s.IsIOException()) {
        return Status::InternalError();
      }
    }
    return s;
  }
};

}  // namespace rocketfs
