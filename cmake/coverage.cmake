
#
# Coverage Setup
#
# Clang recommends to add the coverage flag to all compilation units.
# As otherwise coverage is not tracked.
#
function(ls_setup_coverage target_name do_enable)
    include(cmake/utils.cmake)
    ls_require_bool_value("${do_enable}")

    if ("${do_enable}" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        message(NOTICE "LOGIKSIM: Enabling Clang coverage.")
        target_compile_options("${target_name}" INTERFACE
            -fprofile-instr-generate
            -fcoverage-mapping
        )
        target_link_options("${target_name}" INTERFACE
            -fprofile-instr-generate
            -fcoverage-mapping
        )

    elseif("${do_enable}")
        message(FATAL_ERROR "LOGIKSIM: Coverage only works with Clang.")

    else()
        message(NOTICE "LOGIKSIM: Disabling coverage.")
    endif()

endfunction()

