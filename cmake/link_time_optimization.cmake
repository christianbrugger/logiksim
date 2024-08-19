
function(ls_setup_link_time_optimization do_enable)
    include(cmake/utils.cmake)
    ls_require_bool(do_enable)

    if (do_enable)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT result OUTPUT output)

        if (result)
            message(NOTICE "LOGIKSIM: Enabling link time optimization.")
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message(FATAL_ERROR 
                    "LOGIKSIM: Unable to enable link time optimiaztion." 
                    "Not supported: ${output}")
        endif()

    else()
        message(NOTICE "LOGIKSIM: Disabling link time optimization.")
    endif()

endfunction()

