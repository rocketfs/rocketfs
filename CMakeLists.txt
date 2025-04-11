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

set(CMAKE_CXX_STANDARD 23)
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
  PUBLIC -Wl,--start-group
         asio-grpc::asio-grpc-unifex
         -Wl,--whole-archive
         dpdk
         f-stack
         -Wl,--no-whole-archive
         fmt::fmt-header-only
         gflags::gflags_static
         gRPC::grpc++
         gRPC::grpc++_unsecure
         libfuse
         numa
         protos
         quill::quill
         RocksDB::rocksdb
         unifex::unifex
         -Wl,--end-group)
target_compile_definitions(
  rocketfs
  PUBLIC UNIFEX_NO_ASYNC_STACKS
  PUBLIC UNIFEX_LOG_DANGLING_STOP_CALLBACKS=OFF)

file(GLOB_RECURSE MAIN_SOURCES "src/*_main.cc")
foreach(main_file ${MAIN_SOURCES})
  get_filename_component(file_name ${main_file} NAME_WE)
  string(REPLACE "_main" "" exec_name ${file_name})
  add_executable(${exec_name} ${main_file})
  target_link_libraries(${exec_name} PRIVATE rocketfs)
endforeach()
