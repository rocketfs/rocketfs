add_library(libfuse STATIC IMPORTED)
set_target_properties(
  libfuse
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_LIST_DIR}/libfuse/include
             IMPORTED_LOCATION
             ${CMAKE_CURRENT_LIST_DIR}/libfuse/lib/x86_64-linux-gnu/libfuse3.a)
