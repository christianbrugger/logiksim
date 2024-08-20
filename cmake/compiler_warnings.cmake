
function(ls_get_compiler_warnings RETVAR)

    set(MAIN_COMPILE_OPTIONS)

    # utf-8
    if (MSVC)
        list(APPEND MAIN_COMPILE_OPTIONS /utf-8)
    else()
    endif()

    # our library: needs to be standard complient
    if (MSVC)
        list(APPEND MAIN_COMPILE_OPTIONS /permissive-)
    else()
        list(APPEND MAIN_COMPILE_OPTIONS -pedantic)
    endif()

    # our library: enable warnings
    if (MSVC)
        list(APPEND MAIN_COMPILE_OPTIONS /WX /W4 /external:W0)
        # list(APPEND MAIN_COMPILE_OPTIONS /analyze /analyze:external-)
        
        # list(APPEND MAIN_COMPILE_OPTIONS /we4062 /we4826 /we5204 /we5219 /we5240)
        # list(APPEND MAIN_COMPILE_OPTIONS /we4242 /we4254 /we4287 /we4388)
        # list(APPEND MAIN_COMPILE_OPTIONS /we4263 /we4264 /we4265 /we4266 /we4355)
        # list(APPEND MAIN_COMPILE_OPTIONS /we4296 /we4437 /we4471 /we4545 /we4582 /we4583)
        list(APPEND MAIN_COMPILE_OPTIONS /we5263)

        # list(APPEND MAIN_COMPILE_OPTIONS /we4365)
        
        # disable Spectre mitigation code generation warnings in external files
        list(APPEND MAIN_COMPILE_OPTIONS /wd5045)
        # disable left-to-right evaluation order in braced initializer list
        # list(APPEND MAIN_COMPILE_OPTIONS /wd4868)
        

        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            # unreachable code warnings when using folly::small_vector in release
            list(APPEND MAIN_COMPILE_OPTIONS /wd4702)
        else()
            # disable unused parameters in debug
            list(APPEND MAIN_COMPILE_OPTIONS /wd4100)
        endif()
    else()
        # -Wthread-safety 
        list(APPEND MAIN_COMPILE_OPTIONS -W -Wall -Wextra -Wshadow # -Werror 
                                 -Wstrict-aliasing) # -Wconversion) # -Wsign-conversion)

        # TODO: enable -Werror !!!

        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND 
            (LS_SANITIZE STREQUAL "Address;Undefined" OR LS_SANITIZE STREQUAL "Address"))
            # g++imgenerates those in library headers for sanitized builds
            list(APPEND MAIN_COMPILE_OPTIONS -Wno-maybe-uninitialized)
            # also happens in benchmark which uses Werror
            # TODO: move out of here !!!
            set(BENCHMARK_ENABLE_WERROR OFF)
        endif()
    endif()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # clang 16 warns on brace elision, why?
        list(APPEND MAIN_COMPILE_OPTIONS -Wno-missing-braces)
    endif()


    set("${RETVAR}" "${MAIN_COMPILE_OPTIONS}" PARENT_SCOPE)

endfunction()
