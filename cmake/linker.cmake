
# speedup incremental builds with custom linkers
macro(ls_setup_linker sanitizer_selection)
    set(CMAKE_LINKER_TYPE DEFAULT)

    # Clang-CL Debug
    # For debug build the MSVC linker is much faster then lld-link from clang.
    # Most likely because it supports incremental builds.
    if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
        CMAKE_BUILD_TYPE STREQUAL "Debug" AND
        "${sanitizer_selection}" STREQUAL "")
        set(CMAKE_LINKER_TYPE MSVC)
    endif ()

    # Clang-CL Address Sanitizer is only supported with MSVC linker, not ldd-link
    if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
        "${sanitizer_selection}" STREQUAL "Address")
        set(CMAKE_LINKER_TYPE MSVC)
    endif()

    # On Linux mold linker is faster, if available
    if (UNIX)
        find_program(LS_LINK_PROGRAM_MOLD mold)
        if (LS_LINK_PROGRAM_MOLD)
            set(CMAKE_LINKER_TYPE MOLD)
        endif()
    endif ()

    message(NOTICE "LOGIKSIM: Using linker ${CMAKE_LINKER_TYPE}.")
endmacro()

