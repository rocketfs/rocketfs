set(DPDK_IMPORTED_TARGETS "")

file(GLOB DPDK_LIBRARIES
     ${CMAKE_CURRENT_LIST_DIR}/dpdk/lib/x86_64-linux-gnu/lib*.a)
foreach(lib ${DPDK_LIBRARIES})
  get_filename_component(lib_name ${lib} NAME_WE)
  set(target_name dpdk_${lib_name})
  add_library(${target_name} STATIC IMPORTED)
  set_target_properties(
    ${target_name}
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
               ${CMAKE_CURRENT_LIST_DIR}/dpdk/include IMPORTED_LOCATION ${lib})
  list(APPEND DPDK_IMPORTED_TARGETS ${target_name})
endforeach()

add_library(dpdk INTERFACE)
target_link_libraries(dpdk INTERFACE ${DPDK_IMPORTED_TARGETS})
