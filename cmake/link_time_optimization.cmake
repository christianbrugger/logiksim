
function(ls_setup_link_time_optimization do_enable)
    include(cmake/utils.cmake)
    ls_require_bool(do_enable)

    if (do_enable)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT result OUTPUT output)

        if (result)
            message(NOTICE "LOGIKSIM: Enabling link time optimization.")

            ls_set_global(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

            # some libraries use old cmake versions in requires
            # leading to warnings, e.g. blend2d
            ls_set_global(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
        else()
            message(FATAL_ERROR 
                    "LOGIKSIM: Unable to enable link time optimiaztion." 
                    "Not supported: ${output}")
        endif()

    else()
        message(NOTICE "LOGIKSIM: Disabling link time optimization.")
    endif()

endfunction()

