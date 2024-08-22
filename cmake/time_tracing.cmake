
#
# Enable time tracing for the given target
#
# Note, the flag is only supported by clang
#
function(ls_set_time_tracing target_name do_enable)
    include(cmake/utils.cmake)
    ls_require_bool(do_enable)

    if (do_enable)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            message(NOTICE "LOGIKSIM: Enabling time tracing.")

            target_compile_options("${target_name}" INTERFACE "-ftime-trace")
        else()
            message(FATAL_ERROR "Time tracing only works for Clang!" )
        endif()
    else()
        message(NOTICE "LOGIKSIM: Disabling time tracing.")
    endif()

endfunction()

