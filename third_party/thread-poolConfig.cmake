add_library(libunwind STATIC IMPORTED)
set_target_properties(
  libunwind PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                       ${CMAKE_CURRENT_LIST_DIR}/thread-pool/include)
