#
# Enable color outputs for compilers that support it globally.
#
function(ls_enable_compiler_colors target_name)
    if (MSVC)
        target_compile_options("${target_name}" INTERFACE "/diagnostics:caret")

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options("${target_name}" INTERFACE "-fcolor-diagnostics")

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options("${target_name}" INTERFACE "-fdiagnostics-color=always")
    endif()

endfunction()

