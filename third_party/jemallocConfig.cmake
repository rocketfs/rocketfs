add_library(jemalloc STATIC IMPORTED)
set_target_properties(
  jemalloc
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_LIST_DIR}/jemalloc/include
             IMPORTED_LOCATION
             ${CMAKE_CURRENT_LIST_DIR}/jemalloc/lib/libjemalloc.a)
