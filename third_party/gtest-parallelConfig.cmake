set(GTEST_PARALLEL_EXECUTABLE
    ${CMAKE_CURRENT_LIST_DIR}/gtest-parallel/gtest-parallel/gtest_parallel.py)
add_executable(gtest-parallel IMPORTED)
set_target_properties(gtest-parallel PROPERTIES IMPORTED_LOCATION
                                                ${GTEST_PARALLEL_EXECUTABLE})
