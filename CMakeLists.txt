cmake_minimum_required(VERSION 3.25)

project(
  RocketFS
  VERSION 1.0
  LANGUAGES CXX)

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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(ThirdPartyFetcher)
include(ThirdPartyFinder)
include(ProtobufGenerator)

find_program(IWYU_EXECUTABLE include-what-you-use REQUIRED)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
    ${IWYU_EXECUTABLE}
    -Xiwyu
    --mapping_file=${CMAKE_SOURCE_DIR}/iwyu.json
    -Xiwyu
    --no_comments
    -Xiwyu
    --no_fwd_decls)
find_program(CPPLINT_EXECUTABLE cpplint REQUIRED)
set(CMAKE_CXX_CPPLINT ${CPPLINT_EXECUTABLE}
                      --filter=-whitespace/indent_namespace)
find_program(CLANG_TIDY_EXECUTABLE clang-tidy REQUIRED)
set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXECUTABLE})

include_directories(src)
file(GLOB_RECURSE srcs ${CMAKE_SOURCE_DIR}/src/*.cc)
list(FILTER srcs EXCLUDE REGEX "_main\\.cc$")
add_library(rocketfs STATIC ${srcs})
target_link_libraries(
  rocketfs
  PUBLIC RocksDB::rocksdb
         asio-grpc::asio-grpc-unifex
         fmt::fmt-header-only
         gRPC::grpc++
         gRPC::grpc++_unsecure
         gflags::gflags_static
         protos
         quill::quill
         unifex::unifex)
target_compile_definitions(
  rocketfs
  PUBLIC UNIFEX_NO_ASYNC_STACKS
  PUBLIC UNIFEX_LOG_DANGLING_STOP_CALLBACKS=OFF)

file(GLOB_RECURSE test-srcs ${CMAKE_SOURCE_DIR}/test/*_test.cc)
add_executable(test ${test-srcs})
target_link_libraries(test PUBLIC rocketfs GTest::gtest_main)
