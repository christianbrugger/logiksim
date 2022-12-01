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
    # src/simulation.h src/simulation.cpp 
    src/algorithms.h src/algorithm.cpp
)
target_link_libraries(${MAIN_BENCHMARK_TARGET} PRIVATE 
    benchmark::benchmark
    Boost::boost
)
if (MSVC)
    target_compile_options(${MAIN_BENCHMARK_TARGET} PRIVATE /WX /W4 /permissive- /EHsc)
else()
    target_compile_options(${MAIN_BENCHMARK_TARGET} PRIVATE -Werror -W -Wall -Wextra -Wshadow -pedantic -Wthread-safety -Wfloat-equal -Wstrict-aliasing)
endif()


# MAIN

set(MAIN_TARGET "simulation_srp")
add_executable(${MAIN_TARGET} 
    src/main.cpp
    src/circuit.h src/circuit.cpp
    src/simulation.h src/simulation.cpp 
    src/algorithms.h src/algorithm.cpp
)
target_link_libraries(${MAIN_TARGET} PRIVATE 
    Boost::boost
)
if (MSVC)
    target_compile_options(${MAIN_TARGET} PRIVATE /WX /W4 /permissive- /EHsc)
else()
    target_compile_options(${MAIN_TARGET} PRIVATE -Werror -W -Wall -Wextra -Wshadow -pedantic -Wthread-safety -Wfloat-equal -Wstrict-aliasing)
endif()


# MAIN GUI

set(MAIN_GUI_TARGET "simulation_srp_gui")
add_executable(${MAIN_GUI_TARGET} 
    src/main_gui.cpp
    src/circuit.h src/circuit.cpp
    src/simulation.h src/simulation.cpp 
    src/algorithms.h src/algorithm.cpp
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
    target_compile_options(${MAIN_GUI_TARGET} PRIVATE -Werror -W -Wall -Wextra -Wshadow -pedantic -Wthread-safety -Wfloat-equal -Wstrict-aliasing)
endif()





# profiling
# if (MSVC)
#	target_link_options(${MAIN_TARGET} PRIVATE /PROFILE)
# endif()

# TODO: global optimization / link time optimization

cmake_policy(POP)