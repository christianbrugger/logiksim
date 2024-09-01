

# speedup incremental builds with custom linkers
function(ls_setup_linker sanitizer_selection)

    # Clang-CL Debug
    # For debug build the MSVC linker is much faster then lld-link from clang.
    # Most likely because it supports incremental builds.
    if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
        CMAKE_BUILD_TYPE STREQUAL "Debug" AND
        sanitizer_selection STREQUAL "")
        find_program(LS_LINK_PROGRAM link)
        set(CMAKE_LINKER ${LS_LINK_PROGRAM} PARENT_SCOPE)
    endif ()

    # Clang-CL Address Sanitizer is only supported with MSVC linker, not ldd-link
    if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
        sanitizer_selection STREQUAL "Address")
        find_program(LS_LINK_PROGRAM link)
        set(CMAKE_LINKER ${LS_LINK_PROGRAM} PARENT_SCOPE)
    endif()

    message(NOTICE "LOGIKSIM: Using linker ${CMAKE_LINKER}.")

endfunction()

