cmake_minimum_required(VERSION 3.19)
project(simulation_srp CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


###### GOOGLE BENCHMARK

# # google benchmark
# set(BENCHMARK_ENABLE_TESTING OFF)
# set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
# # set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
# # set(BENCHMARK_ENABLE_LTO ON)
# FetchContent_Declare(
#     googlebenchmark
#     GIT_REPOSITORY https://github.com/google/benchmark
# 	GIT_TAG v1.6.2)
# FetchContent_MakeAvailable(googlebenchmark)

if (MSVC)
else()
	add_compile_options(-std=c++20)
endif()

set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(external/benchmark)

###################################################

# set(CMAKE_CXX_CLANG_TIDY
#   clang-tidy;
#   -format-style='file';
#   -header-filter=${CMAKE_CURRENT_SOURCE_DIR}/src;
# )


set(MAIN_TARGET "simulation_srp")
add_executable(${MAIN_TARGET} src/main.cpp)
target_link_libraries(${MAIN_TARGET} PRIVATE benchmark::benchmark)


