
#
# Setup ccache globally
#
# Ideally the properties set here would be set on the global interface target.
# However, while the properties exist on the target, they are not inherited.
# This macro sets them globally for all targets from here on.
#

macro(ls_setup_ccache do_enable)
    include(cmake/utils.cmake)
    ls_require_bool_value("${do_enable}")

    find_program(LS_CCACHE_PROGRAM ccache)

    if ("${do_enable}" AND LS_CCACHE_PROGRAM)
        message(NOTICE "LOGIKSIM: Enabling ccache with executable '${LS_CCACHE_PROGRAM}'.")

        set(CMAKE_CXX_COMPILER_LAUNCHER 
            "${LS_CCACHE_PROGRAM}"
            # increase sloppiness so it works with pre-compiled headers
            "sloppiness=pch_defines,time_macros,include_file_mtime,include_file_ctime"
            # otherwise pre-processor is run twice on cache misses, increasing build time
            # see https://github.com/ccache/ccache/discussions/1420#discussioncomment-8906839
            "depend_mode=true"
        )

        # CMake default to ProgramDatabase (-Zi) debug information format.
        # This is incompatible with ccache, as it producese a central database.
        # CCache only supports the Embedded (-Z7) format.
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT  
            "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

    elseif("${do_enable}")
        message(WARNING "LOGIKSIM: Warning, requested ccache, but not found.")

    else()
        message(NOTICE "LOGIKSIM: Disabling ccache.")
    endif()
endmacro()

