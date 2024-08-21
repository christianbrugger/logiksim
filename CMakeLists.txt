cmake_minimum_required(VERSION 3.25)

project(simulation_srp C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TODO: investiate this
# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
# set(CMAKE_CXX_EXTENSIONS OFF)

#
# Include
#

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

include(CMakePrintHelpers)

include(cmake/ccache.cmake)
include(cmake/compiler_warnings.cmake)
include(cmake/debug_print.cmake)
include(cmake/link_time_optimization.cmake)
include(cmake/linker.cmake)
include(cmake/sanitizers.cmake)
include(cmake/standard_library.cmake)
include(cmake/time_tracing.cmake)
include(cmake/utils.cmake)


#
# CMake Options
#

option(LS_ENABLE_TIME_TRACE "Trace compilation times. Only works for Clang." OFF)
option(LS_ENABLE_CCACHE "Enable ccache." OFF)
option(LS_ENABLE_LTO "Enable link time optimization." OFF)
option(LS_ENABLE_PCH "Enable pre-compiled headers." ON)
option(LS_ENABLE_COVERAGE "Enable coverage for tests under Clang" OFF)
option(LS_ENABLE_BLEND2D_JIT "Enable JIT for blend2d backend" ON)

# LS_SANITIZE
#    * Address
#    * Undefined
#    * Address;Undefined
#    * Memory
#    * Thread
set(LS_SANITIZE "" CACHE STRING
    "Enable sanitizers (Options: Address Undefined Address;Undefined 
    Memory MemoryWithOrigins Thread). Defaults to empty string.")
set_property(
    CACHE LS_SANITIZE PROPERTY STRINGS 
    "" Address Undefined Address;Undefined Memory Thread
)

# LS_USE_LIBCXX
#    * AUTO - uses libc++ with Memory and Thread sanitizers
#    * ON
#    * OFF
set(LS_USE_LIBCXX AUTO CACHE STRING "Compile all dependencies against clang libc++.")
set_property(CACHE LS_USE_LIBCXX PROPERTY STRINGS AUTO ON OFF)
if ((LS_USE_LIBCXX STREQUAL "AUTO" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
       (LS_SANITIZE STREQUAL "Memory" OR  LS_SANITIZE STREQUAL "MemoryWithOrigins" OR 
        LS_SANITIZE STREQUAL "Thread")) 
    OR (NOT LS_USE_LIBCXX STREQUAL "AUTO" AND LS_USE_LIBCXX)
)
    set(LS_USE_LIBCXX_BOOL TRUE)
else()
    set(LS_USE_LIBCXX_BOOL FALSE)
endif()

ls_print_project_options()

#
# Helper Targets
#

# Options added to logiksim targets and any dependency
add_library(logiksim_global_options INTERFACE)
export(TARGETS logiksim_global_options FILE "${CMAKE_BINARY_DIR}/cmake/LSGlobal.cmake")
# Automatically add it to all newly defined targets
link_libraries(logiksim_global_options)

# Options added to logiksim targets
add_library(logiksim_options INTERFACE)

#
# Project Setup
#

# global options
ls_setup_link_time_optimization("${LS_ENABLE_LTO}")
ls_setup_linker()
ls_setup_ccache("${LS_ENABLE_CCACHE}")

# custom stl & santiziers
ls_setup_standard_library("${LS_USE_LIBCXX_BOOL}")
ls_setup_sanitizers("${LS_SANITIZE}")

ls_set_compiler_warnings(MAIN_COMPILE_OPTIONS)
ls_get_time_tracing(MAIN_COMPILE_OPTIONS "${LS_ENABLE_TIME_TRACE}")

# needed by a lot of tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# print compiler flags
ls_print_compiler_and_flags()
cmake_print_variables(MAIN_COMPILE_OPTIONS)
cmake_print_variables(CMAKE_INTERPROCEDURAL_OPTIMIZATION)

#
# Presets
#

# TODO: move to presets
# global architecture
if (MSVC)
	# add_compile_options(/arch:AVX)
	add_compile_options(/arch:AVX2)
	# add_compile_options(/arch:AVX512)
else()
    # Note that "-march=native" does not lead to improvements in general.
    # See documentation/benchmarks/2024-08-20_compiler_flags_linux_avx.md
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

#
# LogikSim Targets
#

# TODO: remove ?
set(MAIN_LIBRARY "simulation_srp")

# MAIN
add_subdirectory(src)
add_subdirectory(test)

