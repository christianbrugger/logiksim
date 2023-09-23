cmake_minimum_required(VERSION 3.25)

set(LS_CORE_SOURCES
    src/editable_circuit/cache/collision_cache.cpp
    src/editable_circuit/cache/connection_cache.cpp
    src/editable_circuit/cache/spatial_cache.cpp
    src/editable_circuit/cache/split_point_cache.cpp

    src/editable_circuit/cache.cpp
    src/editable_circuit/editable_circuit.cpp
    src/editable_circuit/handler_example.cpp
    src/editable_circuit/handler.cpp
    src/editable_circuit/message.cpp
    src/editable_circuit/sanitizer.cpp
    src/editable_circuit/selection.cpp
    src/editable_circuit/selection_builder.cpp
    src/editable_circuit/selection_registrar.cpp
    src/editable_circuit/type.cpp

    src/algorithm.cpp
    src/circuit_description.cpp
    src/collision.cpp
    src/exception.cpp
    src/file.cpp
    src/format.cpp
    src/geometry.cpp
    src/glyph_cache.cpp
    src/glyph_cache_type.cpp
    src/graph.cpp
    src/interactive_simulation.cpp
    src/layout.cpp
    src/layout_calculation_type.cpp
    src/layout_calculation.cpp
    src/line_tree.cpp
    src/random.cpp
    src/render_benchmark.cpp
    src/render_caches.cpp
    src/render_circuit.cpp
    src/render_generic.cpp
    src/render_helper.cpp
    src/resource.cpp
    src/scene.cpp
    src/schematic.cpp
    src/schematic_generation.cpp
    src/segment_tree.cpp
    src/segment_tree_type.cpp
    src/serialize.cpp
    src/serialize_detail.cpp
    src/setting_handle.cpp
    src/simulation.cpp
    src/simulation_type.cpp
    src/simulation_view.cpp
    src/size_handle.cpp
    src/svg_cache.cpp
    src/text_shaping.cpp
    src/timer.cpp
    src/vocabulary.cpp
    
    src/main_widget.cpp
    src/render_widget.cpp
    src/render_widget_type.cpp
)

# any file that contains Q_OBJECT
set(MOC_FILES
    src/main_widget.h 
    src/render_widget_type.h
)

set(LS_TEST_SOURCES
    test/algorithm_test.cpp
    test/circular_buffer_test.cpp
    test/collision_test.cpp
    test/format_test.cpp
    test/geometry_test.cpp
    test/graph_test.cpp
    test/iterator_adaptor_test.cpp
    test/layout_test.cpp
    test/line_tree_test.cpp
    test/random_test.cpp
    test/range_test.cpp
    test/schematic_test.cpp
    test/segment_tree_test.cpp
    test/simulation_test.cpp
    
    test/editable_circuit/handlers/circuit_item_test.cpp
    test/editable_circuit/handlers/wire_random_test.cpp
    test/editable_circuit/handlers/wire_test.cpp
    test/editable_circuit/editable_circuit_random_test.cpp
)

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

# add_compile_options(-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all) 
# add_link_options(-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)


include(CMakePrintHelpers)
cmake_print_variables(CMAKE_C_COMPILER)
cmake_print_variables(CMAKE_CXX_COMPILER)

option(LS_ENABLE_TIME_TRACE "Trace compilation times. Only works for Clang." OFF)
option(LS_ENABLE_CCACHE "Enable ccache." ON)
option(LS_ENABLE_LTO "Enable link time optimization." OFF)
option(LS_ENABLE_PCH "Enable pre-compiled headers." ON)



# global exceptions
if (MSVC)
	add_compile_options(/EHsc)
else()
	add_compile_options(-fexceptions)
endif()


# global architecture
if (MSVC)
	# add_compile_options(/arch:AVX)
	add_compile_options(/arch:AVX2)
	# add_compile_options(/arch:AVX512)
else()
	add_compile_options(-march=native)
endif()

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
    list(APPEND MAIN_COMPILE_OPTIONS /WX /W4 /external:W0)
    # list(APPEND MAIN_COMPILE_OPTIONS /analyze /analyze:external-)
    
    # list(APPEND MAIN_COMPILE_OPTIONS /we4062 /we4826 /we5204 /we5219 /we5240)
    # list(APPEND MAIN_COMPILE_OPTIONS /we4242 /we4254 /we4287 /we4388)
    # list(APPEND MAIN_COMPILE_OPTIONS /we4263 /we4264 /we4265 /we4266 /we4355)
    # list(APPEND MAIN_COMPILE_OPTIONS /we4296 /we4437 /we4471 /we4545 /we4582 /we4583)
    list(APPEND MAIN_COMPILE_OPTIONS /we5263)

    # list(APPEND MAIN_COMPILE_OPTIONS /we4365)
    
    # disable Spectre mitigation code generation warnings in external files
    list(APPEND MAIN_COMPILE_OPTIONS /wd5045)
    # disable left-to-right evaluation order in braced initializer list
    # list(APPEND MAIN_COMPILE_OPTIONS /wd4868)
    

    # disable unused parameters
    list(APPEND MAIN_COMPILE_OPTIONS /wd4100)
    # unreachable code warnings when using folly::small_vector in release
    list(APPEND MAIN_COMPILE_OPTIONS /wd4702)
else()
    # -Wthread-safety 
    list(APPEND MAIN_COMPILE_OPTIONS -Werror -W -Wall -Wextra -Wshadow 
                             -Wstrict-aliasing) # -Wconversion) # -Wsign-conversion)
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # clang 16 warns on brace elision, why?
    list(APPEND MAIN_COMPILE_OPTIONS -Wno-missing-braces)
endif()

if (LS_ENABLE_LTO)
    message(NOTICE "LOGIKSIM: Enabling link time optimization.")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(NOTICE "LOGIKSIM: Disabling link time optimization.")
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
else()
    message(NOTICE "LOGIKSIM: Disabling ccache.")
endif()
cmake_print_variables(CMAKE_C_FLAGS_DEBUG)
cmake_print_variables(CMAKE_CXX_FLAGS_DEBUG)
cmake_print_variables(CMAKE_C_FLAGS_RELWITHDEBINFO)
cmake_print_variables(CMAKE_CXX_FLAGS_RELWITHDEBINFO)


cmake_print_variables(MAIN_COMPILE_OPTIONS)

###### GOOGLE BENCHMARK

# Google Benchmark
set(BENCHMARK_ENABLE_LTO OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(external/benchmark EXCLUDE_FROM_ALL SYSTEM)

# Google Test
add_subdirectory(external/googletest EXCLUDE_FROM_ALL SYSTEM)

# Zlib
# on Windows
add_subdirectory(external/zlib EXCLUDE_FROM_ALL SYSTEM)
set(ZLIB_USE_STATIC_LIBS ON)
set(LS_ZLIB_NAME zlibstatic)
# on Linux/Ubuntu
# set(LS_ZLIB_NAME )


# Boost
set(BOOST_IOSTREAMS_ENABLE_ZLIB ON)

set(BOOST_INCLUDE_LIBRARIES 
    algorithm 
    container 
    geometry
    iostreams
    mpl 
    random 
    stacktrace
    unordered
)
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
set(CMAKE_AUTOMOC FALSE)  # takes too much time, not parallelized
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
message(NOTICE "Qt version: ${Qt6_VERSION}")

QT6_WRAP_CPP (MOC_SOURCES ${MOC_FILES})

# Blend2D
set(BLEND2D_STATIC TRUE)
add_subdirectory(external/blend2d EXCLUDE_FROM_ALL SYSTEM)

# fmt
add_subdirectory(external/fmt EXCLUDE_FROM_ALL SYSTEM)

# range-v3
add_subdirectory(external/range-v3 EXCLUDE_FROM_ALL SYSTEM)

# GSL
add_subdirectory(external/GSL EXCLUDE_FROM_ALL SYSTEM)

# abseil
set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(external/abseil-cpp EXCLUDE_FROM_ALL SYSTEM)

# Folly - we build a minimal version of the library, only what we need
set(FOLLY_DIR_NAME my_folly)
add_library(folly
    external/${FOLLY_DIR_NAME}/folly/memory/detail/MallocImpl.cpp
    external/${FOLLY_DIR_NAME}/folly/lang/SafeAssert.cpp
    external/${FOLLY_DIR_NAME}/folly/ScopeGuard.cpp
    external/${FOLLY_DIR_NAME}/folly/lang/ToAscii.cpp
)
target_include_directories(folly SYSTEM PUBLIC 
    external/${FOLLY_DIR_NAME}
)
target_compile_definitions(folly PUBLIC FOLLY_NO_CONFIG)

# unordered_dense (ankerl)
add_subdirectory(external/unordered_dense EXCLUDE_FROM_ALL SYSTEM)

# glaze
add_subdirectory(external/glaze EXCLUDE_FROM_ALL SYSTEM)

# cppcodec / base64
add_subdirectory(external/cppcodec EXCLUDE_FROM_ALL SYSTEM)

# harfbuzz
add_subdirectory(external/harfbuzz EXCLUDE_FROM_ALL SYSTEM)

# GCE-Math
add_subdirectory(external/gcem EXCLUDE_FROM_ALL SYSTEM)

# svg2b2d
add_subdirectory(external/svg2b2d EXCLUDE_FROM_ALL SYSTEM)




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
if (LS_ENABLE_PCH)
    message(NOTICE "LOGIKSIM: Enabling pre-compiled headers.")
    target_precompile_headers(logicsim_pch INTERFACE
        <ankerl/unordered_dense.h>
        <blend2d.h>
        <boost/algorithm/string/join.hpp>
        <boost/container/small_vector.hpp>
        <boost/container/vector.hpp>
        <boost/geometry.hpp>
        <boost/random/mersenne_twister.hpp>
        <boost/random/uniform_int_distribution.hpp>
        <fmt/core.h>
        <fmt/format.h>
        <folly/small_vector.h>
        <gcem.hpp>
        <gsl/gsl>
        
        <QWidget>
    
        <algorithm>
        <chrono>
        <cmath>
        <concepts>
        <cstdint>
        <exception>
        <functional>
        <iterator>
        <locale>
        <optional>
        <ostream>
        <queue>
        <random>
        <ranges>
        <ratio>
        <string>
        <string_view>
        <type_traits>
        <utility>
        <variant>
        <vector>
        
        "src/algorithm.h"
        "src/exception.h"
        "src/iterator_adaptor.h"
        "src/format.h"
    )
else()
    message(NOTICE "LOGIKSIM: Disabling pre-compiled headers.")
endif()

set(LS_CORE_INCLUDE_DIRS
    src
)
set(LS_CORE_LIBRARIES  
    logicsim_pch

    Boost::algorithm
    Boost::container 
    Boost::geometry
    Boost::iostreams
    Boost::mpl
    Boost::random
    boost_stacktrace 
    Boost::unordered
    
    cppcodec
    fmt::fmt
    folly
    absl::base
    absl::flat_hash_map
    unordered_dense::unordered_dense
    GSL
    range-v3
    Blend2D::Blend2D
    glaze::glaze
    ${LS_ZLIB_NAME}
    harfbuzz
    gcem
    svg2b2d
    
    Qt6::Core
    Qt6::Widgets
)

# Resources
file(GLOB_RECURSE
    LS_RESOURCE_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/resources/**.ttf    # adapt these to your needs
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/**.svg
)
FOREACH(LS_RESOURCE_FILE ${LS_RESOURCE_FILES})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LS_RESOURCE_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${LS_RESOURCE_FILE}
            ${CMAKE_CURRENT_BINARY_DIR}/${LS_RESOURCE_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${LS_RESOURCE_FILE}
  )
ENDFOREACH()
list(TRANSFORM LS_RESOURCE_FILES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)

# target_compile_options(logicsim_core PUBLIC ${MAIN_COMPILE_OPTIONS})
# target_precompile_headers(logicsim_core PUBLIC external/fmt/include/fmt/format.h)
# target_precompile_headers(logicsim_core PUBLIC external/fmt/include/fmt/ranges.h)
# target_precompile_headers(logicsim_core PUBLIC external/range-v3/include/range/v3/all.hpp)



# MAIN BENCHMARK

set(MAIN_BENCHMARK_TARGET "simulation_srp_benchmark")
add_executable(${MAIN_BENCHMARK_TARGET} 
    ${LS_CORE_SOURCES}
    ${MOC_SOURCES}
    ${LS_RESOURCE_FILES}
    
    src/main_bechmark.cpp
)
target_include_directories(${MAIN_BENCHMARK_TARGET} PRIVATE
    ${LS_CORE_INCLUDE_DIRS}
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
    ${MOC_SOURCES}
    ${LS_RESOURCE_FILES}
    
    src/main.cpp
)
target_include_directories(${MAIN_TARGET} PRIVATE
    ${LS_CORE_INCLUDE_DIRS}
)
target_link_libraries(${MAIN_TARGET} PRIVATE 
    ${LS_CORE_LIBRARIES}
)
target_compile_options(${MAIN_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

# MAIN GUI

set(MAIN_GUI_TARGET "simulation_srp_gui")
add_executable(${MAIN_GUI_TARGET} 
    ${LS_CORE_SOURCES}
    ${MOC_SOURCES}
    ${LS_RESOURCE_FILES}

    src/main_gui.cpp
)
target_include_directories(${MAIN_GUI_TARGET} PRIVATE
    ${LS_CORE_INCLUDE_DIRS}
)
target_link_libraries(${MAIN_GUI_TARGET} PRIVATE 
    ${LS_CORE_LIBRARIES}
)
target_compile_options(${MAIN_GUI_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${MAIN_GUI_TARGET} PRIVATE LS_EXCEPTION_MESSAGE_BOX)
    if (WIN32)
        # set_property(TARGET ${MAIN_GUI_TARGET} PROPERTY WIN32_EXECUTABLE true)
        # target_compile_definitions(${MAIN_GUI_TARGET} PRIVATE LS_LOG_TO_FILE)
    endif ()
endif ()

# Google Test

set(MAIN_TEST_TARGET "simulation_srp_test")
add_executable(${MAIN_TEST_TARGET} 
    ${LS_CORE_SOURCES}
    ${MOC_SOURCES}
    ${LS_TEST_SOURCES}
    ${LS_RESOURCE_FILES}
)
target_include_directories(${MAIN_TEST_TARGET} PRIVATE
    ${LS_CORE_INCLUDE_DIRS}
)
target_link_libraries(${MAIN_TEST_TARGET} PRIVATE
    ${LS_CORE_LIBRARIES}
    
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
)
target_compile_options(${MAIN_TEST_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_BUILD_TYPE MATCHES Debug)
    message(NOTICE "LOGIKSIM: Enabling Clang Coverage for Tests.")
    target_compile_options(${MAIN_TEST_TARGET} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(${MAIN_TEST_TARGET} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
endif()


enable_testing()
include(GoogleTest)
gtest_discover_tests(${MAIN_TEST_TARGET})


# end of file
cmake_policy(POP)
