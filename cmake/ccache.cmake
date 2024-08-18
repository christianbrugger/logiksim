
function(ls_setup_ccache do_enable)
    find_program(LS_CCACHE_PROGRAM ccache)

    if (do_enable AND LS_CCACHE_PROGRAM)
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
endfunction()

