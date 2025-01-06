list(
  APPEND
  CMAKE_PREFIX_PATH
  ${CMAKE_CURRENT_LIST_DIR}/abseil/lib/cmake/absl
  ${CMAKE_CURRENT_LIST_DIR}/asio-grpc/lib/cmake/asio-grpc
  ${CMAKE_CURRENT_LIST_DIR}/curl/lib/cmake/CURL
  ${CMAKE_CURRENT_LIST_DIR}/flatbuffers/lib/cmake/flatbuffers
  ${CMAKE_CURRENT_LIST_DIR}/fmt/lib/cmake/fmt
  ${CMAKE_CURRENT_LIST_DIR}/gflags/lib/cmake/gflags
  ${CMAKE_CURRENT_LIST_DIR}/googletest/lib/cmake/GTest
  ${CMAKE_CURRENT_LIST_DIR}/grpc/lib/cmake/c-ares
  ${CMAKE_CURRENT_LIST_DIR}/grpc/lib/cmake/grpc
  ${CMAKE_CURRENT_LIST_DIR}/grpc/lib/cmake/re2
  ${CMAKE_CURRENT_LIST_DIR}/json/share/cmake/nlohmann_json
  ${CMAKE_CURRENT_LIST_DIR}/libunifex/lib/cmake/unifex
  ${CMAKE_CURRENT_LIST_DIR}/openssl/lib64/cmake/OpenSSL
  ${CMAKE_CURRENT_LIST_DIR}/opentelemetry/lib/cmake/opentelemetry-cpp
  ${CMAKE_CURRENT_LIST_DIR}/prometheus_cpp/lib/cmake/prometheus-cpp
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/lib/cmake/protobuf
  ${CMAKE_CURRENT_LIST_DIR}/protobuf/lib/cmake/utf8_range
  ${CMAKE_CURRENT_LIST_DIR}/rocksdb/lib/cmake/rocksdb
  ${CMAKE_CURRENT_LIST_DIR}/snappy/lib/cmake/Snappy
  ${CMAKE_CURRENT_LIST_DIR}/zstd/lib/cmake/zstd)

set(GTEST_PARALLEL_EXECUTABLE
    ${CMAKE_CURRENT_LIST_DIR}/gtest-parallel/gtest-parallel/gtest_parallel.py)
add_executable(gtest-parallel IMPORTED)
set_target_properties(gtest-parallel PROPERTIES IMPORTED_LOCATION
                                                ${GTEST_PARALLEL_EXECUTABLE})

add_library(jemalloc STATIC IMPORTED)
set_target_properties(
  jemalloc
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_LIST_DIR}/jemalloc/include
             IMPORTED_LOCATION
             ${CMAKE_CURRENT_LIST_DIR}/jemalloc/lib/libjemalloc.a)

add_library(libunwind STATIC IMPORTED)
set_target_properties(
  libunwind
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_LIST_DIR}/libunwind/include
             IMPORTED_LOCATION
             ${CMAKE_CURRENT_LIST_DIR}/libunwind/lib/libunwind.a)

add_library(zlib STATIC IMPORTED)
set_target_properties(
  zlib
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_LIST_DIR}/zlib/include
             IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/zlib/lib/libz.a)
