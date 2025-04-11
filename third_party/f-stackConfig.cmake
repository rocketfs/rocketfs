add_library(f-stack STATIC IMPORTED)
set_target_properties(
  f-stack
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_LIST_DIR}/f-stack/include
             IMPORTED_LOCATION
             ${CMAKE_CURRENT_LIST_DIR}/f-stack/lib/libfstack.a.1.24)
