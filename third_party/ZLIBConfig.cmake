add_library(ZLIB::ZLIB STATIC IMPORTED)
set_target_properties(
  ZLIB::ZLIB
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_LIST_DIR}/zlib/include
             IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/zlib/lib/libz.a)
