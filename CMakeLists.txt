cmake_minimum_required(VERSION 3.31)

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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(IWYU_EXECUTABLE include-what-you-use REQUIRED)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXECUTABLE})

file(GLOB STATUS_RETURN_ANALYZER_SOURCES
     static_analysis/status_return_analyzer/status_return_analyzer.cc)
add_library(status_return_analyzer SHARED ${STATUS_RETURN_ANALYZER_SOURCES})
find_program(LLVM_CONFIG_EXECUTABLE llvm-config REQUIRED)
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
  OUTPUT_VARIABLE LLVM_CXXFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
  OUTPUT_VARIABLE LLVM_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
  OUTPUT_VARIABLE LLVM_LIB_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
target_include_directories(status_return_analyzer SYSTEM
                           PRIVATE ${LLVM_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
target_link_directories(
  status_return_analyzer
  PRIVATE
  ${LLVM_LIB_DIR}
  clangFrontend
  clangAST
  clangASTMatchers
  clangBasic
  clangLex)

file(GLOB TEST_SOURCE static_analysis/status_return_analyzer/test.cc)
add_custom_target(
  test_status_return_analyzer
  COMMAND
    ${CMAKE_CXX_COMPILER} ${TEST_SOURCE} -fsyntax-only -Xclang -load -Xclang
    ${CMAKE_BINARY_DIR}/libstatus_return_analyzer.so -Xclang -plugin -Xclang
    status-return-analyzer
  DEPENDS status_return_analyzer)
