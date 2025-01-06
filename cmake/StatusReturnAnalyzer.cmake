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
