#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
find "${SCRIPT_DIR}" -type d -name "build*" -prune -o -type f \( -name "CMakeLists.txt" -o -name "*.cmake" \) -exec cmake-format -i {} +
find "${SCRIPT_DIR}/src" -type f \( -name "*.h" -o -name "*.cc" \) -exec clang-format -i {} +
find "${SCRIPT_DIR}/test" -type f \( -name "*.h" -o -name "*.cc" \) -exec clang-format -i {} +
