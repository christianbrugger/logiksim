cmake_minimum_required(VERSION 3.25)

include(CMakePrintHelpers)
cmake_print_variables(CMAKE_C_COMPILER )
cmake_print_variables(CMAKE_CXX_COMPILER )

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
  # enfore link time optimization is used when enabled
  cmake_policy(SET CMP0069 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
endif()


project(simulation_srp CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# global flags for all libraries
if (MSVC)
	add_compile_options(/EHsc)
	# add_compile_options(/arch:AVX2)
	# add_link_options(/PROFILE)
else()
	# add_compile_options(-march=native)
	add_compile_options(-fexceptions)
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)


# TODO: global optimization / link time optimization
# TODO enable SIMD
# TODO put flags here

if (MSVC)
    set(MAIN_COMPILE_OPTIONS /permissive- /EHsc)
    list(APPEND MAIN_COMPILE_OPTIONS /WX /W4)

    # unreachable code warnings when using range-v3 templates
    list(APPEND MAIN_COMPILE_OPTIONS /wd4702)
else()
    set(MAIN_COMPILE_OPTIONS -pedantic -fexceptions)
    list(APPEND MAIN_COMPILE_OPTIONS -Werror -W -Wall -Wextra -Wshadow 
                             -Wstrict-aliasing)
		     # -Wthread-safety 
endif()


###### GOOGLE BENCHMARK

# Google Benchmark
# set(BENCHMARK_ENABLE_LTO ON)
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(external/benchmark EXCLUDE_FROM_ALL SYSTEM)

# Google Test
add_subdirectory(external/googletest EXCLUDE_FROM_ALL SYSTEM)

# Boost
# find_package(Boost REQUIRED COMPONENTS)
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

if (false)
    set(CLANG_TIDY_CHECKS "\
    clang-analyzer-*,\
    performance-*,\
    portability-*,\
    concurrency-*,\
    cppcoreguidelines-*,\
    modernize-*,\
    bugprone-*,\
    readability-*,\
    \
    google-default-arguments,\
    google-explicit-constructor,\
    google-readability-avoid-underscore-in-googletest-name,\
    google-runtime-operator,\
    \
    -cppcoreguidelines-avoid-magic-numbers,\
    -readability-magic-numbers,\
    -readability-identifier-length,\
    -readability-convert-member-functions-to-static,\
    -modernize-use-trailing-return-type,\
    -clang-analyzer-optin.cplusplus.UninitializedObject,\
    ")
    set(CMAKE_CXX_CLANG_TIDY
        clang-tidy;
        -extra-arg=/permissive-;
        -extra-arg=/EHsc;
        -header-filter=\\src\\[a-z0-9_]+\.h$;
        -warnings-as-errors=*;
        -checks=${CLANG_TIDY_CHECKS};
    )
endif()




# Main Library

add_library(logicsim_core 
    src/circuit.h src/circuit.cpp
    src/simulation.h src/simulation.cpp 
    src/algorithms.h src/algorithm.cpp
    src/exceptions.h src/exceptions.cpp
    src/timer.h src/timer.cpp
)
target_link_libraries(logicsim_core PUBLIC 
    Boost::container Boost::mpl boost_stacktrace Boost::random
    range-v3::range-v3
    fmt::fmt
    folly
    absl::base
    GSL
)
target_compile_options(logicsim_core PRIVATE ${MAIN_COMPILE_OPTIONS})



# MAIN BENCHMARK

set(MAIN_BENCHMARK_TARGET "simulation_srp_benchmark")
add_executable(${MAIN_BENCHMARK_TARGET} 
    src/main_bechmark.cpp
)
target_link_libraries(${MAIN_BENCHMARK_TARGET} PRIVATE 
    logicsim_core

    benchmark::benchmark
)
target_compile_options(${MAIN_BENCHMARK_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

# MAIN

set(MAIN_TARGET "simulation_srp_main")
add_executable(${MAIN_TARGET} 
    src/main.cpp
    
)
target_link_libraries(${MAIN_TARGET} PRIVATE 
    logicsim_core
)
target_compile_options(${MAIN_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

# MAIN GUI

set(MAIN_GUI_TARGET "simulation_srp_gui")
add_executable(${MAIN_GUI_TARGET} 
    src/render_widget.h src/render_widget.cpp 
    src/render_scene.h src/render_scene.cpp
    src/main_gui.cpp
)
target_link_libraries(${MAIN_GUI_TARGET} PRIVATE 
    logicsim_core

    Qt6::Widgets
    Qt6::OpenGLWidgets
    Blend2D::Blend2D
)
target_compile_options(${MAIN_GUI_TARGET} PRIVATE ${MAIN_COMPILE_OPTIONS})

# Google Test

set(MAIN_TEST_TARGET "simulation_srp_test")
add_executable(${MAIN_TEST_TARGET} 
    test/circuit_test.cpp
    test/random_test.cpp
    test/simulation_test.cpp
)
target_include_directories(${MAIN_TEST_TARGET} PRIVATE
    src
)
target_link_libraries(${MAIN_TEST_TARGET} PRIVATE
    logicsim_core
    
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
