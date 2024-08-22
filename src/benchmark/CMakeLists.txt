
###### Dependencies

# Google Benchmark

# Causes errors in g++ build with address sanitizer
# Last checked: 2024-03-20
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND 
    (LS_SANITIZE STREQUAL "Address;Undefined" OR LS_SANITIZE STREQUAL "Address"))
    set(BENCHMARK_ENABLE_WERROR OFF)
endif()

set(BENCHMARK_ENABLE_LTO OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_INSTALL OFF)
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/benchmark
    ${CMAKE_BINARY_DIR}/external/benchmark
    EXCLUDE_FROM_ALL
    SYSTEM
)


###### Benchmark Executable

set(MAIN_BENCHMARK_TARGET "simulation_srp_benchmark")
add_executable(${MAIN_BENCHMARK_TARGET}
    main_bechmark.cpp
)
target_link_libraries(${MAIN_BENCHMARK_TARGET} PRIVATE 
    ${MAIN_LIBRARY}
    benchmark::benchmark
)
set_property(TARGET ${MAIN_BENCHMARK_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${MAIN_BENCHMARK_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

