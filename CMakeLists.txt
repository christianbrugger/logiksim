cmake_minimum_required(VERSION 3.20)

include(CMakePrintHelpers)
cmake_print_variables(CMAKE_C_COMPILER )
cmake_print_variables(CMAKE_CXX_COMPILER )

cmake_policy(PUSH)

if(POLICY CMP0092)
  cmake_policy(SET CMP0092 NEW) # Don't add -W3 warning level by default.
endif()


project(simulation_srp CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
	add_compile_options(/EHsc)
    # required by range-v3, only allow standard conform behaviour
	# add_compile_options(/permissive-)
	# add_compile_options(/DEBUG /Zi /fsanitize=address)
else()
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

# Google Benchmark
# set(BENCHMARK_ENABLE_LTO ON)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(external/benchmark)

# Google Test
# For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(external/googletest)

# Boost
find_package(Boost REQUIRED COMPONENTS)

# Qt 6
set(CMAKE_AUTOMOC TRUE)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)

# Blend2D
set(BLEND2D_STATIC TRUE)
add_subdirectory(external/blend2d)

# range-v3
add_subdirectory(external/range-v3)

###################################################

# set(CMAKE_CXX_CLANG_TIDY
# 	clang-tidy;
# 	-header-filter=${CMAKE_CURRENT_SOURCE_DIR}/src;
# 	-checks=clang-analyzer-*,performance-*,portability-*,concurrency-*,cppcoreguidelines-*,modernize-*;
# )



# MAIN BENCHMARK

set(MAIN_BENCHMARK_TARGET "simulation_srp_benchmark")
add_executable(${MAIN_BENCHMARK_TARGET} 
    src/main_bechmark.cpp
    src/circuit.h src/circuit.cpp
    src/simulation.h src/simulation.cpp 
    src/algorithms.h src/algorithm.cpp
    src/exceptions.h src/exceptions.cpp
)
target_link_libraries(${MAIN_BENCHMARK_TARGET} PRIVATE 
    benchmark::benchmark
    Boost::boost
    range-v3::range-v3
)
if (MSVC)
    target_compile_options(${MAIN_BENCHMARK_TARGET} PRIVATE /WX /W4 /permissive- /EHsc)
else()
    # -Wfloat-equal
    target_compile_options(${MAIN_BENCHMARK_TARGET} PRIVATE -Werror -W -Wall -Wextra -Wshadow -pedantic -Wthread-safety -Wstrict-aliasing)
endif()


# MAIN

set(MAIN_TARGET "simulation_srp_main")
add_executable(${MAIN_TARGET} 
    src/main.cpp
    src/circuit.h src/circuit.cpp
    src/simulation.h src/simulation.cpp 
    src/algorithms.h src/algorithm.cpp
    src/exceptions.h src/exceptions.cpp
)
target_link_libraries(${MAIN_TARGET} PRIVATE 
    Boost::boost
    range-v3::range-v3
)
if (MSVC)
    target_compile_options(${MAIN_TARGET} PRIVATE /WX /W4 /permissive- /EHsc)
else()
    target_compile_options(${MAIN_TARGET} PRIVATE -Werror -W -Wall -Wextra -Wshadow -pedantic -Wthread-safety -Wstrict-aliasing)
endif()


# MAIN GUI

set(MAIN_GUI_TARGET "simulation_srp_gui")
add_executable(${MAIN_GUI_TARGET} 
    src/main_gui.cpp
    src/circuit.h src/circuit.cpp
    src/simulation.h src/simulation.cpp 
    src/algorithms.h src/algorithm.cpp
    src/exceptions.h src/exceptions.cpp
    src/render_widget.h src/render_widget.cpp 
    src/render_scene.h src/render_scene.cpp
)
target_link_libraries(${MAIN_GUI_TARGET} PRIVATE 
    Boost::boost
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Blend2D::Blend2D
    range-v3::range-v3
)
if (MSVC)
    target_compile_options(${MAIN_GUI_TARGET} PRIVATE /WX /W4 /permissive- /EHsc)
else()
    target_compile_options(${MAIN_GUI_TARGET} PRIVATE -Werror -W -Wall -Wextra -Wshadow -pedantic -Wthread-safety -Wstrict-aliasing)
endif()


# Google Test


set(MAIN_TEST_TARGET "simulation_srp_test")
add_executable(${MAIN_TEST_TARGET} 
    src/circuit.h src/circuit.cpp
    src/simulation.h src/simulation.cpp 
    src/algorithms.h src/algorithm.cpp
    src/exceptions.h src/exceptions.cpp

    test/test_circuit.cpp
    test/test_simulation.cpp
)
target_include_directories(${MAIN_TEST_TARGET} PRIVATE
    src
)
target_link_libraries(${MAIN_TEST_TARGET} PRIVATE
    Boost::boost
    range-v3::range-v3
    
    GTest::gtest
    GTest::gtest_main
)
enable_testing()
include(GoogleTest)
gtest_discover_tests(${MAIN_TEST_TARGET})



# profiling
# if (MSVC)
#	target_link_options(${MAIN_TARGET} PRIVATE /PROFILE)
# endif()

# TODO: global optimization / link time optimization

cmake_policy(POP)