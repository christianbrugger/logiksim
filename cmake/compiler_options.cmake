
#
# Enable UTF-8 sources for the target
#
function(ls_set_compiler_options_utf8 target_name)
    if (MSVC)
        # /utf-8 sets both: /source-charset:utf-8 /execution-charset:utf-8
        target_compile_options("${target_name}" INTERFACE "/utf-8")
    else()
        target_compile_options("${target_name}" INTERFACE "-finput-charset=UTF-8")
        target_compile_options("${target_name}" INTERFACE "-fexec-charset=UTF-8")
    endif()

endfunction()


#
# Only allow standard conformant C++ code
#
function(ls_set_compiler_options_conformant target_name)
    if (MSVC)
        target_compile_options("${target_name}" INTERFACE "/permissive-")
    else()
        target_compile_options("${target_name}" INTERFACE "-pedantic")
    endif()
endfunction()


macro(ls_enabled_shared_library_support)
    # shared libraries require all code to be position independent 
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # on linux symbol visibility is on by default this trashes any shared libraries
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
endmacro()
