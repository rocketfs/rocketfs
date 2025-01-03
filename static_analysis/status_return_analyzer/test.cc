// Copyright 2025 RocketFS

class Status {
 public:
  static Status OK();
  static Status LogicalError();

  bool IsOK() const;
  bool IsLogicalError() const;
};

class Test {
  /**
   * \retval Status::OK() Operation succeeded
   * \retval Status::LogicalError() Logical error occurred
   */
  Status funcA() {
    return Status::OK();  // LogicalError() is not returned
  }

  /**
   * \retval Status::OK() Operation succeeded
   */
  Status funcB() {
    return Status::LogicalError();  // Missing documentation for LogicalError()
  }

  /**
   * \retval Status::OK() Operation succeeded
   */
  Status funcC() { return funcA(); }

  /**
   * \retval Status::OK() Operation succeeded
   */
  Status funcD() {
    const auto s = funcA();
    return s;
  }

  /**
   * \retval Status::OK() Operation succeeded
   */
  Status funcE() {
    const auto s = funcA();
    if (s.IsLogicalError()) {
      return Status::OK();
    }
    return s;
  }

  Status funcF() {
    const auto s = funcA();
    {
      {
        if (s.IsLogicalError()) {
          return Status::OK();
        }
      }
    }
    return s;
  }

  Status funcG() {
    const auto s = funcA();
    if (false) {
      if (s.IsLogicalError()) {
        return Status::OK();
      }
    }
    return s;
  }
};
