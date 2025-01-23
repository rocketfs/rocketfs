cmake_minimum_required(VERSION 3.25)

project(
  RocketFS
  VERSION 1.0
  LANGUAGES CXX)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "RocketFS requires Clang as the compiler.")
endif()
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19)
  message(FATAL_ERROR "RocketFS requires Clang version 19 or higher.")
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
include(ModuleFinder)
include(ProtobufGenerator)
include(StatusReturnAnalyzer)

find_program(IWYU_EXECUTABLE include-what-you-use REQUIRED)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXECUTABLE})
find_program(CPPLINT_EXECUTABLE cpplint REQUIRED)
set(CMAKE_CXX_CPPLINT ${CPPLINT_EXECUTABLE})
find_program(CLANG_TIDY_EXECUTABLE clang-tidy REQUIRED)
set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXECUTABLE})

file(GLOB_RECURSE NAMENODE_SRCS ${CMAKE_SOURCE_DIR}/src/namenode/*.cc)
add_executable(namenode_executable ${NAMENODE_SRCS})
target_link_libraries(
  namenode_executable
  PRIVATE gRPC::grpc++ gRPC::grpc++_unsecure protos asio-grpc::asio-grpc-unifex
          unifex::unifex
  PRIVATE -fsanitize=address,undefined)
target_compile_definitions(
  namenode_executable
  PRIVATE UNIFEX_NO_ASYNC_STACKS
  PRIVATE UNIFEX_LOG_DANGLING_STOP_CALLBACKS=OFF)

file(GLOB_RECURSE CLIENT_SRCS ${CMAKE_SOURCE_DIR}/src/client/*.cc)
add_executable(client_executable ${CLIENT_SRCS})
target_link_libraries(
  client_executable
  PRIVATE gRPC::grpc++ gRPC::grpc++_unsecure protos asio-grpc::asio-grpc-unifex
          unifex::unifex
  PRIVATE -fsanitize=address,undefined)
target_compile_definitions(
  client_executable
  PRIVATE UNIFEX_NO_ASYNC_STACKS
  PRIVATE UNIFEX_LOG_DANGLING_STOP_CALLBACKS=OFF)
