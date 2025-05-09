# This file builds all dependencies for RocketFS, ensuring that RocketFS does
# not need to rebuild all dependencies every time it is built. To maintain
# consistency and readability, we follow the two conventions below:
#
# * `ExternalProject_Add` declarations are sorted in alphabetical order,
#   ignoring case sensitivity.
# * The CMake arguments for each dependency should follow this order:
#
#   * First, arguments related to compilers and installation paths, e.g.,
#     `CMAKE_C_COMPILER` to `CMAKE_INSTALL_PREFIX`.
#   * Next, arguments for dependencies used by the project, e.g.,
#     `-DWITH_ABSEIL=ON` or `-Dabsl_DIR`.
#   * Then, other general configuration options.
#   * Finally, options that disable shared libraries and tests, e.g.,
#     `-Dprotobuf_BUILD_SHARED_LIBS=OFF` and `-Dprotobuf_BUILD_TESTS=OFF`.

cmake_minimum_required(VERSION 3.25)

project(
  RocketFSThirdParty
  VERSION 1.0
  LANGUAGES CXX)

include(ExternalProject)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Please Ensure compilation on a Linux system.")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12.2)
    message(FATAL_ERROR "g++ 12 or greater is required.")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
    message(FATAL_ERROR "Clang++ 19 or greater is required.")
  endif()
else()
  message(FATAL_ERROR "Only g++ and clang++ are supported.")
endif()

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR "CMAKE_C_COMPILER is not set.")
else()
  message(STATUS "CMAKE_C_COMPILER is set to: ${CMAKE_C_COMPILER}.")
endif()
if(NOT CMAKE_CXX_COMPILER)
  message(FATAL_ERROR "CMAKE_CXX_COMPILER is not set.")
else()
  message(STATUS "CMAKE_CXX_COMPILER is set to: ${CMAKE_CXX_COMPILER}.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(ENABLE_UNDEFINED_BEHAVIOR_SANITIZER
    OFF
    CACHE BOOL "Enable undefined behavior sanitizer.")
set(ENABLE_ADDRESS_SANITIZER
    OFF
    CACHE BOOL "Enable address sanitizer.")
set(ENABLE_REALTIME_SANITIZER
    OFF
    CACHE BOOL "Enable realtime sanitizer.")
set(ENABLE_THREAD_SANITIZER
    OFF
    CACHE BOOL "Enable thread sanitizer.")
if(ENABLE_UNDEFINED_BEHAVIOR_SANITIZER
   OR ENABLE_ADDRESS_SANITIZER
   OR ENABLE_REALTIME_SANITIZER
   OR ENABLE_THREAD_SANITIZER)
  set(SANITIZER_FLAGS
      # https://github.com/google/sanitizers/wiki/addresssanitizerflags
      -fno-sanitize-recover=all
      # Leave frame pointers. Allows the fast unwinder to function properly.
      -fno-omit-frame-pointer
      # According to https://gcc.gnu.org/onlinedocs/gcc/Debugging-Options.html,
      # this option allows GCC to embed the compile options into the ELF file.
      # This enables us to verify if sanitizers are enabled. For example:
      # `readelf --string-dump=.GCC.command.line libabsl_spinlock_wait.a`.
      $<$<STREQUAL:${CMAKE_CXX_COMPILER_ID},GNU>:-frecord-gcc-switches>
      $<$<STREQUAL:${CMAKE_CXX_COMPILER_ID},Clang>:-frecord-command-line>
      $<$<BOOL:${ENABLE_UNDEFINED_BEHAVIOR_SANITIZER}>:-fsanitize=undefined>
      $<$<BOOL:${ENABLE_ADDRESS_SANITIZER}>:-fsanitize=address>
      $<$<BOOL:${ENABLE_REALTIME_SANITIZER}>:-fsanitize=realtime>
      $<$<BOOL:${ENABLE_THREAD_SANITIZER}>:-fsanitize=thread>)
  list(JOIN SANITIZER_FLAGS " " SANITIZER_FLAGS)
endif()

function(set_library_paths library_name)
  string(TOUPPER ${library_name} upper_library_name)
  string(REPLACE "-" "_" upper_library_name ${upper_library_name})
  set(${upper_library_name}_PREFIX
      ${CMAKE_BINARY_DIR}/thirdparty/${library_name}
      PARENT_SCOPE)
  set(${upper_library_name}_OUTPUT
      ${CMAKE_BINARY_DIR}/output/${library_name}
      PARENT_SCOPE)
endfunction()
set_library_paths(abseil)
set_library_paths(asio-grpc)
set_library_paths(curl)
set_library_paths(flatbuffers)
set_library_paths(fmt)
set_library_paths(libfuse)
set_library_paths(gflags)
set_library_paths(googletest)
set_library_paths(grpc)
set_library_paths(gtest-parallel)
set_library_paths(jemalloc)
set_library_paths(json)
set_library_paths(libunifex)
set_library_paths(libunwind)
set_library_paths(magic_enum)
set_library_paths(openssl)
set_library_paths(opentelemetry)
set_library_paths(prometheus_cpp)
set_library_paths(protobuf)
set_library_paths(quill)
set_library_paths(rocksdb)
set_library_paths(snappy)
set_library_paths(thread-pool)
set_library_paths(zlib)
set_library_paths(zstd)

ExternalProject_Add(
  abseil
  PREFIX ${ABSEIL_PREFIX}
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20240722.0
  GIT_SHALLOW ON
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${ABSEIL_OUTPUT}
             -DABSL_PROPAGATE_CXX_STD=ON
             -DABSL_BUILD_TESTING=OFF
             -DABSL_USE_GOOGLETEST_HEAD=OFF
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  asio-grpc
  PREFIX ${ASIO_GRPC_PREFIX}
  GIT_REPOSITORY https://github.com/Tradias/asio-grpc.git
  GIT_TAG v3.4.1
  GIT_SHALLOW ON
  CMAKE_ARGS # -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             # -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${ASIO_GRPC_OUTPUT}
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  curl
  PREFIX ${CURL_PREFIX}
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG curl-8_11_1
  GIT_SHALLOW ON
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${CURL_OUTPUT}
             -DOPENSSL_ROOT_DIR=${OPENSSL_OUTPUT}
             -DBUILD_SHARED_LIBS=OFF
             -DBUILD_STATIC_LIBS=ON
             -DBUILD_STATIC_CURL=ON
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)
add_dependencies(curl openssl)

ExternalProject_Add(
  flatbuffers
  PREFIX ${FLATBUFFERS_PREFIX}
  GIT_REPOSITORY https://github.com/google/flatbuffers.git
  GIT_TAG v24.12.23
  GIT_SHALLOW ON
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${FLATBUFFERS_OUTPUT}
             -DFLATBUFFERS_CPP_STD=${CMAKE_CXX_STANDARD}
             -DFLATBUFFERS_LIBCXX_WITH_CLANG=OFF
             -DFLATBUFFERS_BUILD_TESTS=OFF
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  fmt
  PREFIX ${FMT_PREFIX}
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.1.1
  GIT_SHALLOW ON
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${FMT_OUTPUT}
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  libfuse
  PREFIX ${LIBFUSE_PREFIX}
  GIT_REPOSITORY https://github.com/libfuse/libfuse.git
  GIT_TAG fuse-3.16.2
  GIT_SHALLOW ON
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
    meson setup ${LIBFUSE_PREFIX}/src/libfuse
  COMMAND
    meson configure -Dbuildtype=release -Ddefault_library=static
    -Dprefer_static=true -Dc_args=${SANITIZER_FLAGS}
    -Dc_link_args=${SANITIZER_FLAGS} -Dcpp_std=c++20
    -Dcpp_args=${SANITIZER_FLAGS} -Dcpp_link_args=${SANITIZER_FLAGS}
    -Dprefix=${LIBFUSE_OUTPUT}
  COMMAND meson setup --reconfigure ${LIBFUSE_PREFIX}/src/libfuse
  BUILD_COMMAND ninja
  INSTALL_COMMAND ninja install
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  gflags
  PREFIX ${GFLAGS_PREFIX}
  GIT_REPOSITORY https://github.com/gflags/gflags.git
  GIT_TAG v2.2.2
  GIT_SHALLOW ON
  CMAKE_ARGS # -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             # -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${GFLAGS_OUTPUT}
             -DBUILD_STATIC_LIBS=ON
             -DBUILD_TESTING=OFF
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  googletest
  PREFIX ${GOOGLETEST_PREFIX}
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
  GIT_SHALLOW ON
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${GOOGLETEST_OUTPUT}
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  grpc
  PREFIX ${GRPC_PREFIX}
  GIT_REPOSITORY https://github.com/grpc/grpc.git
  GIT_TAG v1.69.0
  GIT_SHALLOW ON
  GIT_SUBMODULES_RECURSE OFF
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${GRPC_OUTPUT}
             -DgRPC_ABSL_PROVIDER=package
             -Dabsl_DIR=${ABSEIL_OUTPUT}/lib/cmake/absl
             -DgRPC_OPENTELEMETRY_PROVIDER=package
             -DgRPC_PROTOBUF_PROVIDER=package
             -DProtobuf_DIR=${PROTOBUF_OUTPUT}/lib/cmake/protobuf
             -Dutf8_range_DIR=${PROTOBUF_OUTPUT}/lib/cmake/utf8_range
             -DgRPC_SSL_PROVIDER=package
             -DOPENSSL_ROOT_DIR=${OPENSSL_OUTPUT}
             -DgRPC_ZLIB_PROVIDER=package
             -DZLIB_INCLUDE_DIR=${ZLIB_OUTPUT}/include
             -DZLIB_LIBRARY=${ZLIB_OUTPUT}/lib/libz.a
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)
add_dependencies(grpc abseil protobuf openssl zlib)

ExternalProject_Add(
  gtest-parallel
  PREFIX ${GTEST_PARALLEL_PREFIX}
  GIT_REPOSITORY https://github.com/google/gtest-parallel.git
  GIT_TAG 96f4f904922f9bf66689e749c40f314845baaac8
  GIT_SHALLOW ON
  CONFIGURE_COMMAND echo
  BUILD_COMMAND echo
  INSTALL_COMMAND rm -rf ${GTEST_PARALLEL_OUTPUT}
  COMMAND mkdir -p ${GTEST_PARALLEL_OUTPUT}
  COMMAND cp -r ${GTEST_PARALLEL_PREFIX}/src/gtest-parallel
          ${GTEST_PARALLEL_OUTPUT})

# According to https://github.com/jemalloc/jemalloc/issues/2353, Jemalloc and
# Address Sanitizer are not compatible.
ExternalProject_Add(
  jemalloc
  PREFIX ${JEMALLOC_PREFIX}
  GIT_REPOSITORY https://github.com/jemalloc/jemalloc.git
  GIT_TAG 5.3.0
  GIT_SHALLOW ON
  BUILD_IN_SOURCE ON
  CONFIGURE_COMMAND sh autogen.sh
  COMMAND
    ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
    ./configure --prefix=${JEMALLOC_OUTPUT} --enable-prof
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  json
  PREFIX ${JSON_PREFIX}
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW ON
  CMAKE_ARGS # -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             # -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${JSON_OUTPUT})

ExternalProject_Add(
  libunifex
  PREFIX ${LIBUNIFEX_PREFIX}
  GIT_REPOSITORY https://github.com/rocketfs/libunifex.git
  GIT_TAG ed9739883cc143050acfe7c3e7cceaeffbea5543
  GIT_SHALLOW ON
  CMAKE_ARGS # -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             # -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${LIBUNIFEX_OUTPUT}
             -DBUILD_TESTING=OFF
             -DUNIFEX_BUILD_EXAMPLES=OFF)

ExternalProject_Add(
  libunwind
  PREFIX ${LIBUNWIND_PREFIX}
  GIT_REPOSITORY https://github.com/libunwind/libunwind.git
  GIT_TAG v1.8.1
  GIT_SHALLOW ON
  BUILD_IN_SOURCE ON
  CONFIGURE_COMMAND autoreconf -i
  COMMAND
    ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
    ./configure --disable-shared --enable-static --prefix=${LIBUNWIND_OUTPUT}
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  magic_enum
  PREFIX ${MAGIC_ENUM_PREFIX}
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG v0.9.7
  GIT_SHALLOW ON
  CMAKE_ARGS # -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             # -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${MAGIC_ENUM_OUTPUT}
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

# https://github.com/openssl/openssl/blob/master/INSTALL.md
ExternalProject_Add(
  openssl
  PREFIX ${OPENSSL_PREFIX}
  GIT_REPOSITORY https://github.com/openssl/openssl.git
  GIT_TAG openssl-3.4.0
  GIT_SHALLOW ON
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -E env CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
    CFLAGS=${SANITIZER_FLAGS} CXXFLAGS=${SANITIZER_FLAGS}
    ${OPENSSL_PREFIX}/src/openssl/Configure no-shared
    --with-zlib-include=${ZLIB_OUTPUT}/include
    --with-zlib-lib=${ZLIB_OUTPUT}/lib --openssldir=${OPENSSL_OUTPUT}
    --prefix=${OPENSSL_OUTPUT})
add_dependencies(openssl zlib)

ExternalProject_Add(
  opentelemetry
  PREFIX ${OPENTELEMETRY_PREFIX}
  GIT_REPOSITORY https://github.com/open-telemetry/opentelemetry-cpp.git
  GIT_TAG v1.18.0
  GIT_SHALLOW ON
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${OPENTELEMETRY_OUTPUT}
             -DWITH_ABSEIL=ON
             -Dabsl_DIR=${ABSEIL_OUTPUT}/lib/cmake/absl
             -DCURL_INCLUDE_DIR=${CURL_OUTPUT}/include
             -DCURL_LIBRARY=${CURL_OUTPUT}/lib/libcurl.a
             -DgRPC_DIR=${GRPC_OUTPUT}/lib/cmake/grpc
             -DOPENSSL_ROOT_DIR=${OPENSSL_OUTPUT}
             -DProtobuf_DIR=${PROTOBUF_OUTPUT}/lib/cmake/protobuf
             -Dutf8_range_DIR=${PROTOBUF_OUTPUT}/lib/cmake/utf8_range
             -DZLIB_INCLUDE_DIR=${ZLIB_OUTPUT}/include
             -DZLIB_LIBRARY=${ZLIB_OUTPUT}/lib/libz.a
             -DWITH_OPENTRACING=OFF
             -DWITH_OTLP_GRPC=ON
             -DWITH_OTLP_FILE=ON
             -DWITH_PROMETHEUS=ON
             -DBUILD_TESTING=OFF
             -DWITH_FUNC_TESTS=OFF
             -DWITH_BENCHMARK=OFF
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)
add_dependencies(
  opentelemetry
  abseil
  curl
  grpc
  openssl
  protobuf
  zlib)

ExternalProject_Add(
  prometheus-cpp
  PREFIX ${PROMETHEUS_CPP_PREFIX}
  GIT_REPOSITORY https://github.com/jupp0r/prometheus-cpp.git
  GIT_TAG v1.3.0
  GIT_SHALLOW ON
  GIT_SUBMODULES_RECURSE OFF
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${PROMETHEUS_CPP_OUTPUT}
             -DENABLE_PUSH=OFF
             -DENABLE_COMPRESSION=OFF
             -DTHIRDPARTY_CIVETWEB_WITH_SSL=OFF
             -DENABLE_TESTING=OFF
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  protobuf
  PREFIX ${PROTOBUF_PREFIX}
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v29.2
  GIT_SHALLOW ON
  GIT_SUBMODULES_RECURSE OFF
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${PROTOBUF_OUTPUT}
             -Dprotobuf_LOCAL_DEPENDENCIES_ONLY=ON
             -Dprotobuf_ABSL_PROVIDER=package
             -Dabsl_DIR=${ABSEIL_OUTPUT}/lib/cmake/absl
             -Dprotobuf_WITH_ZLIB=ON
             -DZLIB_INCLUDE_DIR=${ZLIB_OUTPUT}/include
             -DZLIB_LIBRARY=${ZLIB_OUTPUT}/lib/libz.a
             -Dprotobuf_BUILD_SHARED_LIBS=OFF
             -Dprotobuf_BUILD_TESTS=OFF
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)
add_dependencies(protobuf abseil zlib)

ExternalProject_Add(
  quill
  PREFIX ${QUILL_PREFIX}
  GIT_REPOSITORY https://github.com/odygrd/quill.git
  GIT_TAG v8.0.0
  GIT_SHALLOW ON
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${QUILL_OUTPUT}
             -DQUILL_NO_EXCEPTIONS=ON
             -DQUILL_ENABLE_INSTALL=ON)

ExternalProject_Add(
  rocksdb
  PREFIX ${ROCKSDB_PREFIX}
  GIT_REPOSITORY https://github.com/facebook/rocksdb.git
  GIT_TAG v9.10.0
  GIT_SHALLOW ON
  GIT_SUBMODULES_RECURSE OFF
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             # https://github.com/facebook/rocksdb/blob/main/INSTALL.md
             # ... a reasonable compromise is`PORTABLE=haswell` ...
             -DPORTABLE=haswell
             -DCMAKE_INSTALL_PREFIX=${ROCKSDB_OUTPUT}
             -DWITH_GFLAGS=ON
             -Dgflags_DIR=${GFLAGS_OUTPUT}/lib/cmake/gflags
             -DWITH_SNAPPY=ON
             -DSnappy_INCLUDE_DIRS=${SNAPPY_OUTPUT}/include
             -DSnappy_LIBRARIES=${SNAPPY_OUTPUT}/lib/libsnappy.a
             -DWITH_ZSTD=ON
             -DZSTD_INCLUDE_DIRS=${ZSTD_OUTPUT}/include
             -DZSTD_LIBRARIES=${ZSTD_OUTPUT}/lib/libzstd.a
             -DFAIL_ON_WARNINGS=OFF
             -DROCKSDB_BUILD_SHARED=OFF
             -DUSE_RTTI=ON
             -DWITH_TOOLS=ON
             -DWITH_BENCHMARK_TOOLS=OFF
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)
add_dependencies(rocksdb gflags snappy zstd)

ExternalProject_Add(
  snappy
  PREFIX ${SNAPPY_PREFIX}
  GIT_REPOSITORY https://github.com/google/snappy.git
  GIT_TAG 1.2.1
  GIT_SHALLOW ON
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${SNAPPY_OUTPUT}
             -DSNAPPY_BUILD_TESTS=OFF
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  thread-pool
  PREFIX ${THREAD_POOL_PREFIX}
  GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
  GIT_TAG v5.0.0
  GIT_SHALLOW ON
  CONFIGURE_COMMAND echo
  BUILD_COMMAND echo
  INSTALL_COMMAND rm -rf ${THREAD_POOL_OUTPUT}
  COMMAND mkdir -p ${THREAD_POOL_OUTPUT}
  COMMAND cp -r ${THREAD_POOL_PREFIX}/src/thread-pool/include
          ${THREAD_POOL_OUTPUT}
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  zlib
  PREFIX ${ZLIB_PREFIX}
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG v1.3.1
  GIT_SHALLOW ON
  BUILD_IN_SOURCE ON
  CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DCMAKE_C_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DCMAKE_BUILD_TYPE=Release
             -DCMAKE_INSTALL_PREFIX=${ZLIB_OUTPUT}
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

ExternalProject_Add(
  zstd
  PREFIX ${ZSTD_PREFIX}
  GIT_REPOSITORY https://github.com/facebook/zstd.git
  GIT_TAG v1.5.6
  GIT_SHALLOW ON
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -S <SOURCE_DIR>/build/cmake -B <BINARY_DIR>
    -G${CMAKE_GENERATOR} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_FLAGS=${SANITIZER_FLAGS} -DCMAKE_CXX_FLAGS=${SANITIZER_FLAGS}
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=${ZSTD_OUTPUT}
  LOG_CONFIGURE ON
  LOG_BUILD ON
  LOG_INSTALL ON
  LOG_OUTPUT_ON_FAILURE ON)

install(
  FILES ${CMAKE_SOURCE_DIR}/gtest-parallelConfig.cmake
        ${CMAKE_SOURCE_DIR}/jemallocConfig.cmake
        ${CMAKE_SOURCE_DIR}/libfuseConfig.cmake
        ${CMAKE_SOURCE_DIR}/libunwindConfig.cmake
        ${CMAKE_SOURCE_DIR}/thread-poolConfig.cmake
        ${CMAKE_SOURCE_DIR}/Targets.cmake
        ${CMAKE_SOURCE_DIR}/ZLIBConfig.cmake
  DESTINATION ${CMAKE_BINARY_DIR}/output)
