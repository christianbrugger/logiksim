cmake_minimum_required(VERSION 3.25)

include(CMakePrintHelpers)
cmake_print_variables(CMAKE_C_COMPILER)
cmake_print_variables(CMAKE_CXX_COMPILER)

cmake_policy(PUSH)

if(POLICY CMP0092)
  # Don't add -W3 warning level by default.
  cmake_policy(SET CMP0092 NEW)
endif()
if(POLICY CMP0141)
  # don't use 'MSVC_DEBUG_INFORMATION_FORMAT (BOOST bug 2022-12-14)  # TODO check 
  cmake_policy(SET CMP0141 OLD)
endif()
if(POLICY CMP0069)
  # enforce link time optimization is used when enabled
  cmake_policy(SET CMP0069 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
endif()


project(simulation_srp CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(LS_ENABLE_TIME_TRACE "Trace compilation times. Only works for Clang." OFF)
option(LS_ENABLE_CCACHE "Enable ccache." ON)
option(LS_ENABLE_LTO "Enable link time optimization." ON)


# global exceptions
if (MSVC)
	add_compile_options(/EHsc)
else()
	add_compile_options(-fexceptions)
endif()


# global architecture
#if (MSVC)
#	# add_compile_options(/arch:AVX2)
#else()
#	# add_compile_options(-march=native)
#endif()

# global profiling
#if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# add_link_options(/PROFILE)
#endif()

# our library: needs to be standard complient
set(MAIN_COMPILE_OPTIONS)
if (MSVC)
    list(APPEND MAIN_COMPILE_OPTIONS /permissive-)
else()
    list(APPEND MAIN_COMPILE_OPTIONS -pedantic)
endif()

# our library: enable warnings
if (MSVC)
    list(APPEND MAIN_COMPILE_OPTIONS /WX /W4)
    # unreachable code warnings when using range-v3 templates
    list(APPEND MAIN_COMPILE_OPTIONS /wd4702)
else()
	# -Wthread-safety 
    list(APPEND MAIN_COMPILE_OPTIONS -Werror -W -Wall -Wextra -Wshadow 
                             -Wstrict-aliasing)
endif()

if (LS_ENABLE_LTO)
    message(NOTICE "LOGIKSIM: Enabling link time optimization.")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (LS_ENABLE_TIME_TRACE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(NOTICE "LOGIKSIM: Enabling time tracing.")
        list(APPEND MAIN_COMPILE_OPTIONS -ftime-trace)
    else()
        message(FATAL_ERROR "Time tracing only works for Clang!" )
    endif()
endif()

if (LS_ENABLE_CCACHE)
    message(NOTICE "LOGIKSIM: Enabling ccache.")
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    # ccache only supports /Z7, not /Zi
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()
cmake_print_variables(CMAKE_C_FLAGS_DEBUG)
cmake_print_variables(CMAKE_CXX_FLAGS_DEBUG)
cmake_print_variables(CMAKE_C_FLAGS_RELWITHDEBINFO)
cmake_print_variables(CMAKE_CXX_FLAGS_RELWITHDEBINFO)


cmake_print_variables(MAIN_COMPILE_OPTIONS)

###### GOOGLE BENCHMARK

# Google Benchmark
# set(BENCHMARK_ENABLE_LTO ON)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(external/benchmark EXCLUDE_FROM_ALL SYSTEM)

# Google Test
add_subdirectory(external/googletest EXCLUDE_FROM_ALL SYSTEM)

# Boost
set(BOOST_INCLUDE_LIBRARIES container stacktrace mpl random algorithm)
add_subdirectory(external/boost EXCLUDE_FROM_ALL SYSTEM)
if (WIN32)
    # Boost::stacktrace_windbg
    add_library(boost_stacktrace ALIAS boost_stacktrace_windbg)
    target_compile_definitions(boost_stacktrace_windbg PRIVATE 
        BOOST_STACKTRACE_USE_WINDBG
    )
else()
    # Boost::stacktrace_basic
    add_library(boost_stacktrace ALIAS boost_stacktrace_basic)   
endif()


# Qt 6
set(CMAKE_AUTOMOC TRUE)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)

# Blend2D
set(BLEND2D_STATIC TRUE)  # TODO test BLEND2D_EMBED
add_subdirectory(external/blend2d EXCLUDE_FROM_ALL SYSTEM)

# range-v3
add_subdirectory(external/range-v3 EXCLUDE_FROM_ALL SYSTEM)

# fmt
add_subdirectory(external/fmt EXCLUDE_FROM_ALL SYSTEM)

# GSL
add_subdirectory(external/GSL EXCLUDE_FROM_ALL SYSTEM)

# abseil
set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(external/abseil-cpp EXCLUDE_FROM_ALL SYSTEM)
  
# Folly - we build a mimimal version of the library, only what we need
add_library(folly
    external/folly/folly/memory/detail/MallocImpl.cpp
)
target_include_directories(folly SYSTEM PUBLIC 
    external/folly
)
target_compile_definitions(folly PUBLIC FOLLY_NO_CONFIG)




###################################################
# needed to run clang-tidy outside, generates compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (false)
    set(CMAKE_CXX_CLANG_TIDY
        clang-tidy;
        -extra-arg=/permissive-;
        -extra-arg=/EHsc;
        -warnings-as-errors=*;
        -checks=${CLANG_TIDY_CHECKS};
    )
    #-header-filter=\\src\\[a-z0-9_]+\.h$;
endif()




# Main Library

add_library(logicsim_pch INTERFACE)
target_precompile_headers(logicsim_pch INTERFACE
    <boost/algorithm/string/join.hpp>
    <boost/random/uniform_int_distribution.hpp>
    <fmt/core.h>
    <gsl/gsl>
    <range/v3/range/concepts.hpp>
    
    <algorithm>
    <chrono>
    <concepts>
    <cstdint>
    <exception>
    <iostream>
    <iterator>
    <optional>
    <random>
    <ranges>
    <string>
    <type_traits>
    <utility>
    <vector>
)


set(LS_CORE_SOURCES
    src/algorithm.h src/algorithm.cpp
    src/circuit.h src/circuit.cpp
    src/exceptions.h src/exceptions.cpp
    src/random.h src/random.cpp
    src/simulation.h src/simulation.cpp
    src/timer.h src/timer.cpp
	src/newlib.cpp
)
set(LS_CORE_LIBRARIES  
    logicsim_pch

    Boost::algorithm
    Boost::container 
    Boost::mpl
    Boost::random
    boost_stacktrace 

    range-v3::range-v3
    fmt::fmt
    folly
    absl::base
    GSL
)

# target_compile_options(logicsim_core PUBLIC ${MAIN_COMPILE_OPTIONS})
# target_precompile_headers(logicsim_core PUBLIC external/fmt/include/fmt/format.h)
# target_precompile_headers(logicsim_core PUBLIC external/fmt/include/fmt/ranges.h)
# target_precompile_headers(logicsim_core PUBLIC external/range-v3/include/range/v3/all.hpp)



# MAIN BENCHMARK

set(MAIN_BENCHMARK_TARGET "simulation_srp_benchmark")
add_executable(${MAIN_BENCHMARK_TARGET} 
    ${LS_CORE_SOURCES}
    
    src/main_bechmark.cpp
)
target_link_libraries(${MAIN_BENCHMARK_TARGET} PRIVATE 
    ${LS_CORE_LIBRARIES}

    benchmark::benchmark
)
target_compile_options(${MAIN_BENCHMARK_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

# MAIN

set(MAIN_TARGET "simulation_srp_main")
add_executable(${MAIN_TARGET} 
    ${LS_CORE_SOURCES}

    src/main.cpp
    
)
target_link_libraries(${MAIN_TARGET} PRIVATE 
    ${LS_CORE_LIBRARIES}
)
target_compile_options(${MAIN_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

# MAIN GUI

set(MAIN_GUI_TARGET "simulation_srp_gui")
add_executable(${MAIN_GUI_TARGET} 
    ${LS_CORE_SOURCES}

    src/render_widget.h src/render_widget.cpp 
    src/render_scene.h src/render_scene.cpp
    src/main_gui.cpp
)
target_link_libraries(${MAIN_GUI_TARGET} PRIVATE 
    ${LS_CORE_LIBRARIES}

    Qt6::Widgets
    Qt6::OpenGLWidgets
    Blend2D::Blend2D
)
target_compile_options(${MAIN_GUI_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

# Google Test

set(MAIN_TEST_TARGET "simulation_srp_test")
add_executable(${MAIN_TEST_TARGET} 
    ${LS_CORE_SOURCES}
    
    test/circuit_test.cpp
    test/random_test.cpp
    test/range_test.cpp
    test/simulation_test.cpp
)
target_include_directories(${MAIN_TEST_TARGET} PRIVATE
    src
)
target_link_libraries(${MAIN_TEST_TARGET} PRIVATE
    ${LS_CORE_LIBRARIES}
    
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
)
target_compile_options(${MAIN_TEST_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

enable_testing()
include(GoogleTest)
gtest_discover_tests(${MAIN_TEST_TARGET})


# end of file
cmake_policy(POP)
