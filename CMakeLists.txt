cmake_minimum_required(VERSION 3.25)

project(simulation_srp C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


#
# CMake Options
#

option(LS_ENABLE_TIME_TRACE "Trace compilation times. Only works for Clang." OFF)
option(LS_ENABLE_CCACHE "Enable ccache." OFF)
option(LS_ENABLE_LTO "Enable link time optimization." OFF)
option(LS_ENABLE_PCH "Enable pre-compiled headers." ON)
option(LS_ENABLE_COVERAGE "Enable coverage for tests under Clang" OFF)

option(LS_BLEND2D_NO_JIT "Disable JIT for blend2d backend" OFF)
option(LS_WARNINGS_AS_ERRORS "Report warnings as errors and abort the build" ON)

# LS_SANITIZE
#    * Address
#    * Undefined
#    * Address;Undefined
#    * Memory
#    * MemoryWithOrigins
#    * Thread
set(LS_SANITIZE "" CACHE STRING
    "Enable sanitizers (Options: Address Undefined Address;Undefined 
    Memory MemoryWithOrigins Thread). Defaults to empty string.")
set_property(
    CACHE LS_SANITIZE PROPERTY STRINGS 
    "" Address Undefined Address;Undefined Memory MemoryWithOrigins Thread
)

# LS_USE_LIBCXX
#   Memory and Thread sanitizers require the c++ standard library to rebuild.
#   Use clang libc++ in this case.
option(LS_USE_LIBCXX "Compile clang libc++ from source and link everything with it." OFF)


#
# Includes
#

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

include(CMakePrintHelpers)

include(cmake/ccache.cmake)
include(cmake/compiler_colors.cmake)
include(cmake/compiler_options.cmake)
include(cmake/compiler_warnings.cmake)
include(cmake/debug_print.cmake)
include(cmake/link_time_optimization.cmake)
include(cmake/linker.cmake)
include(cmake/sanitizers.cmake)
include(cmake/standard_library.cmake)
include(cmake/time_tracing.cmake)
include(cmake/utils.cmake)


#
# Helper Targets
#

# Options added to logiksim targets and any dependency
add_library(logiksim_global_options INTERFACE)
export(TARGETS logiksim_global_options FILE "${CMAKE_BINARY_DIR}/cmake/LSGlobal.cmake")

# Options added to logiksim targets
add_library(logiksim_options INTERFACE)


#
# Project Setup
#

ls_print_project_options()


# global cmake options
ls_setup_linker("${LS_SANITIZE}")
ls_setup_link_time_optimization("${LS_ENABLE_LTO}")
ls_setup_ccache("${LS_ENABLE_CCACHE}")


# global target options
ls_enable_compiler_colors(logiksim_global_options)
# static linking is not supported by TSAN
ls_setup_standard_library_shared(
    logiksim_global_options 
    "${LS_USE_LIBCXX}" 
    "${LS_SANITIZE}"
)
ls_setup_sanitizers(logiksim_global_options "${LS_SANITIZE}")
# Automatically add it to all new targets from here on
link_libraries(logiksim_global_options)


# options for logiksim targets
ls_set_compiler_options_utf8(logiksim_options)
ls_set_compiler_options_conformant(logiksim_options)
ls_set_compiler_warnings(logiksim_options)
ls_set_compiler_warnings_disabled(logiksim_options)
ls_set_compiler_warnings_as_errors(logiksim_options "${LS_WARNINGS_AS_ERRORS}")
ls_set_time_tracing(logiksim_options "${LS_ENABLE_TIME_TRACE}")


# needed by a lot of tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# print compiler flags
ls_print_compiler_and_flags()
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


#
# LogikSim Targets
#

# TODO: remove ?
set(MAIN_LIBRARY "simulation_srp")

# MAIN
add_subdirectory(src)

