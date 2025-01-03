cmake_minimum_required(VERSION 3.31)

project(RocketIO VERSION 1.0 LANGUAGES CXX)

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "RocketIO requires Clang as the compiler.")
endif()
if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19)
    message(FATAL_ERROR "RocketIO requires Clang version 19 or higher.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_program(LLVM_CONFIG_EXECUTABLE llvm-config REQUIRED)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags OUTPUT_VARIABLE LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir OUTPUT_VARIABLE LLVM_LIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

file(GLOB STATUS_RETURN_ANALYZER_SOURCES
    static_analysis/status_return_analyzer/status_return_analyzer.cc
)
include_directories(${LLVM_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR})
link_directories(${LLVM_LIB_DIR})
add_library(status_return_analyzer SHARED ${STATUS_RETURN_ANALYZER_SOURCES})
target_link_libraries(status_return_analyzer
    clangFrontend
    clangAST
    clangASTMatchers
    clangBasic
    clangLex
    # fmt
)

file(GLOB TEST_SOURCE static_analysis/status_return_analyzer/test.cc)
add_custom_target(run_status_return_analyzer
    COMMAND clang++-19 ${TEST_SOURCE} -fsyntax-only
            -Xclang -load -Xclang ${CMAKE_BINARY_DIR}/libstatus_return_analyzer.so
            -Xclang -plugin -Xclang status-return-analyzer
    DEPENDS ${CMAKE_BINARY_DIR}/libstatus_return_analyzer.so
    VERBATIM
)