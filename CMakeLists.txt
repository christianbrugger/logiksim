cmake_minimum_required(VERSION 3.19)
project(simulation_srp CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#if (MSVC)
#else()
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#	set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
#endif()



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
    add_compile_options(/DEBUG /Zi /fsanitize=address)
else()
	add_compile_options(-std=c++20)
endif()

set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
# add_subdirectory(external/benchmark)

###################################################

# set(CMAKE_CXX_CLANG_TIDY
#   clang-tidy;
#   -header-filter=${CMAKE_CURRENT_SOURCE_DIR}/src;
#   -checks=clang-analyzer-*,performance-*,portability-*,concurrency-*,cppcoreguidelines-*,modernize-*;
# )


set(MAIN_TARGET "simulation_srp")
add_executable(${MAIN_TARGET} src/main2.cpp)
# target_link_libraries(${MAIN_TARGET} PRIVATE benchmark::benchmark)

if (MSVC)
    # optimizations
	target_compile_options(${MAIN_TARGET} PRIVATE /O2)
    # warnings
    target_compile_options(${MAIN_TARGET} PRIVATE /W4 /permissive-)
	
    target_compile_options(${MAIN_TARGET} PRIVATE /DEBUG /Zi /fsanitize=address)
	
	# target_compile_options(${MAIN_TARGET} PRIVATE /MT)
	# target_link_options(${MAIN_TARGET} PRIVATE /MT)
else()
	# target_link_options(${MAIN_TARGET} PRIVATE -fuse-ld=lld)
    # optimization
#	target_compile_options(${MAIN_TARGET} PRIVATE -O3)
	# warnings
#	target_compile_options(${MAIN_TARGET} PRIVATE -W -Wall -Wextra -pedantic -Wthread-safety)
	
	#target_compile_options(${MAIN_TARGET} PRIVATE  -fno-omit-frame-pointer)# -static # -fsanitize=address
	                                               #-D_DISABLE_VECTOR_ANNOTATION -D_DISABLE_STRING_ANNOTATION)
	#target_link_options(${MAIN_TARGET} PRIVATE -fno-omit-frame-pointer) # -static) # -fsanitize=address  
	                                           #-D_DISABLE_VECTOR_ANNOTATION -D_DISABLE_STRING_ANNOTATION)
endif()

# TODO: global optimization / link time optimization
