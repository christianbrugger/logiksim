cmake_minimum_required(VERSION 3.25)

project(simulation_srp C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
# set(CMAKE_CXX_EXTENSIONS OFF)




include(CMakePrintHelpers)
cmake_print_variables(CMAKE_C_COMPILER)
cmake_print_variables(CMAKE_CXX_COMPILER)

option(LS_ENABLE_TIME_TRACE "Trace compilation times. Only works for Clang." OFF)
option(LS_ENABLE_CCACHE "Enable ccache." OFF)
option(LS_ENABLE_LTO "Enable link time optimization." OFF)
option(LS_ENABLE_PCH "Enable pre-compiled headers." ON)
option(LS_ENABLE_COVERAGE "Enable coverage for tests under Clang" OFF)

# option for sanitizers
set(LS_SANITIZE "" CACHE STRING
    "Enable sanitizers (Options: Address Undefined Address;Undefined 
    Memory MemoryWithOrigins Thread). Defaults to empty string.")
set_property(
    CACHE LS_SANITIZE PROPERTY STRINGS 
    "" Address Undefined Address;Undefined Memory Thread
)

# option for libc++
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

#message("TEST LS_SANITIZE = ${LS_SANITIZE}")
#message("TEST LS_USE_LIBCXX = ${LS_USE_LIBCXX}")
#message("TEST LS_USE_LIBCXX_BOOL = ${LS_USE_LIBCXX_BOOL}")



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


set(MAIN_COMPILE_OPTIONS)

# utf-8
if (MSVC)
	list(APPEND MAIN_COMPILE_OPTIONS /utf-8)
else()
endif()

# our library: needs to be standard complient
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
    

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        # unreachable code warnings when using folly::small_vector in release
        list(APPEND MAIN_COMPILE_OPTIONS /wd4702)
    else()
        # disable unused parameters in debug
        list(APPEND MAIN_COMPILE_OPTIONS /wd4100)
    endif()
else()
    # -Wthread-safety 
    list(APPEND MAIN_COMPILE_OPTIONS -W -Wall -Wextra -Wshadow -Werror 
                             -Wstrict-aliasing) # -Wconversion) # -Wsign-conversion)


    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND 
        (LS_SANITIZE STREQUAL "Address;Undefined" OR LS_SANITIZE STREQUAL "Address"))
        # g++ generates those in library headers for sanitized builds
        list(APPEND MAIN_COMPILE_OPTIONS -Wno-maybe-uninitialized)
        # also happens in benchmark which uses Werror
        set(BENCHMARK_ENABLE_WERROR OFF)
    endif()
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

# setup ccache
find_program(LS_CCACHE_PROGRAM ccache)
if (LS_ENABLE_CCACHE AND LS_CCACHE_PROGRAM)
    message(NOTICE "LOGIKSIM: Enabling ccache.")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${LS_CCACHE_PROGRAM})
    # ccache only supports /Z7, not /Zi
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
else()
    message(NOTICE "LOGIKSIM: Disabling ccache.")
endif()

# select linker
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # for debug build the MSVC linker is much faster then lld-link from clang
    find_program(LS_LINK_PROGRAM link)
    set(CMAKE_LINKER ${LS_LINK_PROGRAM})
endif ()
message(NOTICE "LOGIKSIM: Using linker ${CMAKE_LINKER}.")




cmake_print_variables(CMAKE_C_FLAGS_DEBUG)
cmake_print_variables(CMAKE_CXX_FLAGS_DEBUG)
cmake_print_variables(CMAKE_C_FLAGS_RELWITHDEBINFO)
cmake_print_variables(CMAKE_CXX_FLAGS_RELWITHDEBINFO)


cmake_print_variables(MAIN_COMPILE_OPTIONS)





list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")


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

# set(BLEND2D_NO_JIT ON)


###################################################
# needed to run clang-tidy outside, generates compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# if (false)
#     set(CMAKE_CXX_CLANG_TIDY
#         clang-tidy;
#         -extra-arg=/permissive-;
#         -extra-arg=/EHsc;
#         -warnings-as-errors=*;
#         -checks=${CLANG_TIDY_CHECKS};
#     )
#     #-header-filter=\\src\\[a-z0-9_]+\.h$;
# endif()


# TODO: remove ?
set(MAIN_LIBRARY "simulation_srp")

# MAIN
add_subdirectory(src)
add_subdirectory(test)

