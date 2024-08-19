cmake_minimum_required(VERSION 3.25)

project(simulation_srp C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TODO: investiate this
# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
# set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

include(CMakePrintHelpers)

include(cmake/ccache.cmake)
include(cmake/compiler_warnings.cmake)
include(cmake/debug_print.cmake)
include(cmake/link_time_optimization.cmake)
include(cmake/linker.cmake)
include(cmake/project_options.cmake)
include(cmake/sanitizers.cmake)
include(cmake/standard_library.cmake)
include(cmake/time_tracing.cmake)
include(cmake/utils.cmake)


# options
ls_define_program_options()
ls_print_program_options()

# lto, linker, ccache
ls_setup_link_time_optimization("${LS_ENABLE_LTO}")
ls_setup_linker()
ls_setup_ccache("${LS_ENABLE_CCACHE}")

# custom stl & santiziers
ls_setup_standard_library("${LS_USE_LIBCXX_BOOL}")
ls_setup_sanitizers("${LS_SANITIZE}")

ls_get_compiler_warnings(MAIN_COMPILE_OPTIONS)
ls_get_time_tracing(MAIN_COMPILE_OPTIONS "${LS_ENABLE_TIME_TRACE}")

# needed by a lot of tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# TODO: move to presets
# global architecture
if (MSVC)
	# add_compile_options(/arch:AVX)
	add_compile_options(/arch:AVX2)
	# add_compile_options(/arch:AVX512)
else()
	add_compile_options(-march=native)
endif()


if (NOT MSVC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND 
        (LS_SANITIZE STREQUAL "Address;Undefined" OR LS_SANITIZE STREQUAL "Address"))
        # g++imgenerates those in library headers for sanitized builds
        # also happens in benchmark which uses Werror
        set(BENCHMARK_ENABLE_WERROR OFF)
    endif()
endif()

# TODO: to presets?
# better debugging profile
if (CMAKE_BUILD_TYPE STREQUAL "Debug")  # AND LS_SANITIZE
    if (MSVC)
        # add_compile_options(/O1)
    else()
        add_compile_options(-O1 -fno-optimize-sibling-calls)
    endif()
endif()


# print compiler flags
ls_print_compiler_and_flags()
cmake_print_variables(MAIN_COMPILE_OPTIONS)


# TODO: remove ?
set(MAIN_LIBRARY "simulation_srp")

# MAIN
add_subdirectory(src)
add_subdirectory(test)

