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
include(cmake/utils.cmake)


ls_define_program_options()
ls_print_program_options()

ls_setup_link_time_optimization(${LS_ENABLE_LTO})
ls_setup_linker()
ls_setup_ccache(${LS_ENABLE_CCACHE})

ls_get_compiler_warnings(MAIN_COMPILE_OPTIONS)

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

# global profiling
#if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# add_link_options(/PROFILE)
#endif()


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

# setup time tracing
if (LS_ENABLE_TIME_TRACE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(NOTICE "LOGIKSIM: Enabling time tracing.")
        list(APPEND MAIN_COMPILE_OPTIONS -ftime-trace)
    else()
        message(FATAL_ERROR "Time tracing only works for Clang!" )
    endif()
endif()





# print compiler flags
ls_print_compiler_and_flags()
cmake_print_variables(MAIN_COMPILE_OPTIONS)



if (LS_USE_LIBCXX_BOOL)
    message(NOTICE "LOGIKSIM: Using clang libc++.")

    set(LLVM_ENABLE_RUNTIMES "libcxx;libcxxabi;libunwind" CACHE STRING "" FORCE)

    #set(LIBCXX_ENABLE_SHARED OFF)
    #set(LIBCXXABI_ENABLE_SHARED OFF)
    #set(LIBCXX_ENABLE_STATIC ON)
    #set(LIBCXXABI_ENABLE_STATIC ON)

    set(LIBCXX_ENABLE_SHARED ON)
    set(LIBCXXABI_ENABLE_SHARED ON)
    set(LIBCXX_ENABLE_STATIC OFF)
    set(LIBCXXABI_ENABLE_STATIC OFF)

    set(LLVM_USE_SANITIZER ${LS_SANITIZE})
    add_subdirectory(external/llvm-project/runtimes EXCLUDE_FROM_ALL SYSTEM)
    link_libraries(
        #cxx_static
        #cxxabi_static
        cxx_shared
        cxxabi_shared
        #cxx_experimental
    )

    # The google benchmark contains an unconditional export that requires
    # all its dependencies to be exportable. By itself llvm does not define
    # export for runtime targets. Thats why we define them here.
    export(TARGETS 
        cxx_shared cxxabi_shared cxx-headers cxxabi_shared_objects 
        libcxx-abi-headers cxxabi-headers 
        FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/LlvmRuntimeTargets.cmake"
    )
else()
    message(NOTICE "LOGIKSIM: Using default stdlib.")
endif()


# TODO -fsanitize=float-divide-by-zero

# Set Sanitizer flags
#
# Set these flags after adding clang libc++. For those we pass
# our sanitizer config and the generic settings don't work.
#
if (LS_SANITIZE STREQUAL "")
elseif(LS_SANITIZE STREQUAL "Address")
    add_compile_options(-fsanitize=address) 
    add_link_options(-fsanitize=address)
elseif(LS_SANITIZE STREQUAL "Undefined")
    add_compile_options(-fsanitize=undefined) 
    add_link_options(-fsanitize=undefined)
elseif(LS_SANITIZE STREQUAL "Address;Undefined")
    add_compile_options(-fsanitize=address -fsanitize=undefined) 
    add_link_options(-fsanitize=address -fsanitize=undefined)
elseif(LS_SANITIZE STREQUAL "Memory")
    add_compile_options(-fsanitize=memory)
    add_link_options(-fsanitize=memory)
elseif(LS_SANITIZE STREQUAL "MemoryWithOrigins")
    add_compile_options(-fsanitize=memory -fsanitize-memory-track-origins) 
    add_link_options(-fsanitize=memory -fsanitize-memory-track-origins)
elseif(LS_SANITIZE STREQUAL "Thread")
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
    set(BLEND2D_NO_JIT ON)
else()
    message(FATAL_ERROR "Unknown LS_SANITIZE Option: ${LS_SANITIZE}")
endif()
if (LS_SANITIZE)
    add_compile_options(-fno-sanitize-recover=all -fno-omit-frame-pointer -g) 
    add_link_options(-fno-sanitize-recover=all)
endif()


# TODO: remove ?
set(MAIN_LIBRARY "simulation_srp")

# MAIN
add_subdirectory(src)
add_subdirectory(test)

