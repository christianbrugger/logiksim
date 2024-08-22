
#
# Setup ccache globally
#
macro(ls_setup_ccache do_enable)
    include(cmake/utils.cmake)
    ls_require_bool_value("${do_enable}")

    find_program(LS_CCACHE_PROGRAM ccache)

    if ("${do_enable}" AND LS_CCACHE_PROGRAM)
        message(NOTICE "LOGIKSIM: Enabling ccache with executable '${LS_CCACHE_PROGRAM}'.")

        set(CMAKE_CXX_COMPILER_LAUNCHER "${LS_CCACHE_PROGRAM}")

        # TODO; review, can we do this another way
        # ccache only supports /Z7, not /Zi
        string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
        string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    elseif("${do_enable}")
        message(WARNING "LOGIKSIM: Warning, requested ccache, but not found.")

    else()
        message(NOTICE "LOGIKSIM: Disabling ccache.")
    endif()
endmacro()

