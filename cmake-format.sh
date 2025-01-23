#!/bin/bash

set -euo pipefail

find . -type d -name "build*" -prune -o -type f \( -name "CMakeLists.txt" -o -name "*.cmake" \) -exec cmake-format -i {} +
