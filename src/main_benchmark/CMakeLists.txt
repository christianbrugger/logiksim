
###### Dependencies

# Google Benchmark

# Benchmark is not error clean on windows or sanitizer builds.
# We can't really do much about it, so we disable errors as warnings for the library.
set(BENCHMARK_ENABLE_WERROR OFF)

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

add_executable(${LS_BENCHMARK_TARGET}
    main_benchmark.cpp
)
target_link_libraries(${LS_BENCHMARK_TARGET} PRIVATE 
    ${LS_CORE_LIBRARY}
    benchmark::benchmark
)
set_property(TARGET ${LS_BENCHMARK_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${LS_BENCHMARK_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

