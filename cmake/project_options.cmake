

function(ls_define_program_options)
    option(LS_ENABLE_TIME_TRACE "Trace compilation times. Only works for Clang." OFF)
    option(LS_ENABLE_CCACHE "Enable ccache." OFF)
    option(LS_ENABLE_LTO "Enable link time optimization." OFF)
    option(LS_ENABLE_PCH "Enable pre-compiled headers." ON)
    option(LS_ENABLE_COVERAGE "Enable coverage for tests under Clang" OFF)

    # sanitizers
    set(LS_SANITIZE "" CACHE STRING
        "Enable sanitizers (Options: Address Undefined Address;Undefined 
        Memory MemoryWithOrigins Thread). Defaults to empty string.")
    set_property(
        CACHE LS_SANITIZE PROPERTY STRINGS 
        "" Address Undefined Address;Undefined Memory Thread
    )

    # libc++
    set(LS_USE_LIBCXX AUTO CACHE STRING "Compile all dependencies against clang libc++.")
    set_property(CACHE LS_USE_LIBCXX PROPERTY STRINGS AUTO ON OFF)

    if ((LS_USE_LIBCXX STREQUAL "AUTO" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
           (LS_SANITIZE STREQUAL "Memory" OR  LS_SANITIZE STREQUAL "MemoryWithOrigins" OR 
            LS_SANITIZE STREQUAL "Thread")) 
        OR (NOT LS_USE_LIBCXX STREQUAL "AUTO" AND LS_USE_LIBCXX)
    )
        set(LS_USE_LIBCXX_BOOL_result TRUE)
    else()
        set(LS_USE_LIBCXX_BOOL_result FALSE)
    endif()

    set(LS_USE_LIBCXX_BOOL ${LS_USE_LIBCXX_BOOL_result} CACHE BOOL "Compile with clang libc++")

    # message("TEST LS_SANITIZE = ${LS_SANITIZE}")
    # message("TEST LS_USE_LIBCXX = ${LS_USE_LIBCXX}")
    # message("TEST LS_USE_LIBCXX_BOOL = ${LS_USE_LIBCXX_BOOL}")
    #
endfunction()

function(ls_print_program_options)
    include(CMakePrintHelpers)

    cmake_print_variables(LS_ENABLE_TIME_TRACE)
    cmake_print_variables(LS_ENABLE_CCACHE)
    cmake_print_variables(LS_ENABLE_LTO)
    cmake_print_variables(LS_ENABLE_PCH)
    cmake_print_variables(LS_ENABLE_COVERAGE)

    cmake_print_variables(LS_SANITIZE)
    cmake_print_variables(LS_USE_LIBCXX)
    cmake_print_variables(LS_USE_LIBCXX_BOOL)
endfunction()
