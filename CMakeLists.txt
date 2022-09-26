cmake_minimum_required(VERSION 3.20)

cmake_policy(PUSH)

if(POLICY CMP0092)
  cmake_policy(SET CMP0092 NEW) # Don't add -W3 warning level by default.
endif()


project(simulation_srp CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
	add_compile_options(/EHsc)
	# add_compile_options(/DEBUG /Zi /fsanitize=address)
else()
	# add_compile_options(-std=c++20)
endif()


###### GOOGLE BENCHMARK

# # google benchmark
# include(FetchContent)
# set(BENCHMARK_ENABLE_TESTING OFF)
# set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
# # set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
# # set(BENCHMARK_ENABLE_LTO ON)
# FetchContent_Declare(
#     googlebenchmark
#     GIT_REPOSITORY https://github.com/google/benchmark
# 	GIT_TAG v1.6.2)
# FetchContent_MakeAvailable(googlebenchmark)

# set(BENCHMARK_ENABLE_LTO ON)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(external/benchmark)

find_package(Boost REQUIRED COMPONENTS)

###################################################

# set(CMAKE_CXX_CLANG_TIDY
# 	clang-tidy;
# 	-header-filter=${CMAKE_CURRENT_SOURCE_DIR}/src;
# 	-checks=clang-analyzer-*,performance-*,portability-*,concurrency-*,cppcoreguidelines-*,modernize-*;
# )

set(MAIN_TARGET "simulation_srp")
add_executable(${MAIN_TARGET} 
	src/main.cpp
    src/simulation.h
 "src/simulation.cpp" "src/algorithms.h" "src/algorithm.cpp")
target_link_libraries(${MAIN_TARGET} PRIVATE 
	benchmark::benchmark
	Boost::boost
)
# warning flags
if (MSVC)
    target_compile_options(${MAIN_TARGET} PRIVATE /WX /W4 /permissive- /EHsc)
else()
    target_compile_options(${MAIN_TARGET} PRIVATE -Werror -W -Wall -Wextra -Wshadow -pedantic -Wthread-safety -Wfloat-equal -Wstrict-aliasing)
endif()

# strict-aliasing

# TODO: global optimization / link time optimization

cmake_policy(POP)