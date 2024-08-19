

# speedup incremental builds with custom linkers
function(ls_setup_linker)

    # Windows & Debug & Clang
    if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # for debug build the MSVC linker is much faster then lld-link from clang
        find_program(LS_LINK_PROGRAM link)
        set(CMAKE_LINKER ${LS_LINK_PROGRAM})
    endif ()

    message(NOTICE "LOGIKSIM: Using linker ${CMAKE_LINKER}.")

endfunction()

