
function(ls_get_time_tracing RETVAR do_enable)
    include(cmake/utils.cmake)
    ls_require_bool(do_enable)

    if (do_enable)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            message(NOTICE "LOGIKSIM: Enabling time tracing.")
            # TODO: a bit of a hack !!!
            list(APPEND "${RETVAR}" "-ftime-trace")
        else()
            message(FATAL_ERROR "Time tracing only works for Clang!" )
        endif()
    else()
        message(NOTICE "LOGIKSIM: Disabling time tracing.")
    endif()

    set("${RETVAR}" "${${RETVAR}}" PARENT_SCOPE) 

endfunction()
