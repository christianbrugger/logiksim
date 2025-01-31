cmake_minimum_required(VERSION 3.30)

project(simulation_srp C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# enable shared library build
# TODO move to cmake folder
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)


#
# CMake Options
#

option(LS_ENABLE_TIME_TRACE "Trace compilation times. Only works for Clang." OFF)
option(LS_ENABLE_CCACHE "Enable ccache." OFF)
option(LS_ENABLE_LTO "Enable link time optimization." OFF)
option(LS_ENABLE_PCH "Enable pre-compiled headers." ON)
option(LS_ENABLE_COVERAGE "Enable coverage for tests under Clang" OFF)
option(LS_GUI_SAMPLE "Only configure the GUI sample project" OFF)

option(LS_BLEND2D_NO_JIT "Disable JIT for blend2d backend" OFF)
option(LS_WARNINGS_AS_ERRORS "Report warnings as errors and abort the build" ON)
option(LS_WIN32_APPLICATON "Do not build as console application on windows." OFF)

# LS_SANITIZE
#    * Address
#    * Undefined
#    * Address;Undefined
#    * Address;Fuzzer
#    * Memory
#    * MemoryWithOrigins
#    * Thread
set(LS_SANITIZE "" CACHE STRING
    "Enable sanitizers (Options: Address Undefined Address;Undefined Address;Fuzzer
    Memory MemoryWithOrigins Thread). Defaults to empty string.")
set_property(
    CACHE LS_SANITIZE PROPERTY STRINGS 
    "" Address Undefined Address;Undefined Address;Fuzzer Memory MemoryWithOrigins Thread
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
include(cmake/coverage.cmake)
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
ls_setup_coverage(logiksim_global_options "${LS_ENABLE_COVERAGE}")
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

set(LS_CORE_LIBRARY "ls_core_lib")
set(LS_CORE_SHARED "ls_core_shared")
set(LS_CORE_STATIC "ls_core_static")
set(LS_GUI_LIBRARY "ls_gui_lib")

set(LS_VERIFY_CORE_SHARED_C "ls_verify_core_shared_c")
set(LS_VERIFY_CORE_SHARED_CPP "ls_verify_core_shared_cpp")

set(LS_BENCHMARK_TARGET "ls_benchmark")
set(LS_CLI_TARGET "ls_cli")
set(LS_GUI_TARGET "ls_gui")
set(LS_SAMPLE_GUI_TARGET "ls_gui_sample")

set(LS_CORE_TEST_TARGET "ls_test_core")
set(LS_GUI_TEST_TARGET "ls_test_gui")

set(LS_FUZZ_CORE "ls_fuzz_core")


# MAIN
add_subdirectory(example_circuits)
add_subdirectory(resources)
add_subdirectory(src)

