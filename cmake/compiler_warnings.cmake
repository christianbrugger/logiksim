
#
# Treat warnings as errors on the given target
#
function(ls_set_compiler_warnings_as_errors target_name)
    if (MSVC)
        target_compile_options("${target_name}" INTERFACE "/WX")
    else()
        target_compile_options("${target_name}" INTERFACE "-Werror")
    endif()
endfunction()


#
# Enable common warnings on the given target
#
function(ls_set_compiler_warnings target_name)
    set(warnings "")

    if (MSVC)
        # disable warnings in external header files
        # does not work for the C47XX backend warnings
        # does not work for code analysis violations
        list(APPEND warnings /external:anglebrackets /external:W0)

        # base level
        list(APPEND warnings /W4)

        # require explicit [[fallthrough]] for case
        list(APPEND warnings /we5262)
        # calling 'std::move' on a temporary object prevents copy elision
        list(APPEND warnings /we5263)
        
        # list(APPEND warnings /analyze /analyze:external-)

        # list(APPEND warnings /we4062 /we4826 /we5204 /we5219 /we5240)
        # list(APPEND warnings /we4242 /we4254 /we4287 /we4388)
        # list(APPEND warnings /we4263 /we4264 /we4265 /we4266 /we4355)
        # list(APPEND warnings /we4296 /we4437 /we4471 /we4545 /we4582 /we4583)

        # list(APPEND warnings /we4365)
        
        # disable Spectre mitigation code generation warnings in external files
        list(APPEND warnings /wd5045)
        # disable left-to-right evaluation order in braced initializer list
        # list(APPEND warnings /wd4868)
        

        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            # unreachable code warnings when using folly::small_vector in release
            list(APPEND warnings /wd4702)
        else()
            # disable unused parameters in debug
            list(APPEND warnings /wd4100)
        endif()
    endif()

    if (NOT MSVC)
        # base level
        list(APPEND warnings -Wall)
        # standard extensions
        list(APPEND warnings -Wextra)

        # variable shadows something in parent scope
        list(APPEND warnings -Wshadow)
        # violate optimizer strict aliasing rules
        list(APPEND warnings -Wstrict-aliasing)
        
        # -Wconversion
        # -Wsign-conversion)
        # -Wthread-safety 

        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND 
            (LS_SANITIZE STREQUAL "Address;Undefined" OR LS_SANITIZE STREQUAL "Address"))
            # g++imgenerates those in library headers for sanitized builds
            list(APPEND warnings -Wno-maybe-uninitialized)
            # also happens in benchmark which uses Werror
            # TODO: move out of here !!!
            set(BENCHMARK_ENABLE_WERROR OFF)
        endif()
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # clang 16 warns on brace elision, why?
        list(APPEND warnings -Wno-missing-braces)
    endif()

    target_compile_options("${target_name}" INTERFACE ${warnings})
endfunction()
