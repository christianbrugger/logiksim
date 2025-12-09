
#
# Treat warnings as errors on the given target
#
function(ls_set_compiler_warnings_as_errors target_name do_enable)
    include(cmake/utils.cmake)
    ls_require_bool(do_enable)

    if (do_enable)
        message(NOTICE "LOGIKSIM: Enabling warnings as errors.")
        if (MSVC)
            target_compile_options("${target_name}" INTERFACE "/WX")
        else()
            target_compile_options("${target_name}" INTERFACE "-Werror")
        endif()
    else()
        message(NOTICE "LOGIKSIM: Disabling warnings as errors.")
    endif()

endfunction()


#
# Enable common warnings on the given target
#
function(ls_set_compiler_warnings target_name)
    set(warnings "")

    if (MSVC)
        # disable warnings in external header files
        #  * does not work for the C47XX backend warnings
        #  * does not work for code analysis violations
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            list(APPEND warnings /external:anglebrackets /external:W0)
        else()
            # clang-cl does not support this flag
            list(APPEND warnings /external:W0)
        endif()

        # base level
        list(APPEND warnings /W4)

        # initialization needs to be in order for classes
        list(APPEND warnings /w15038)
        # require explicit [[fallthrough]] for case
        list(APPEND warnings /w15262)
        # calling 'std::move' on a temporary object prevents copy elision
        list(APPEND warnings /w15263)
        
        # list(APPEND warnings /analyze /analyze:external-)

        # list(APPEND warnings /we4062 /we4826 /we5204 /we5219 /we5240)
        # list(APPEND warnings /we4242 /we4254 /we4287 /we4388)
        # list(APPEND warnings /we4263 /we4264 /we4265 /we4266 /we4355)
        # list(APPEND warnings /we4296 /we4437 /we4471 /we4545 /we4582 /we4583)

        # list(APPEND warnings /we4365)


        # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        list(APPEND warnings /w14242)
        # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits'
        list(APPEND warnings /w14254)
        # 'function': member function does not override any base class virtual member
        list(APPEND warnings /w14263)
        # 'classname': class has virtual functions, but destructor is not virtual
        list(APPEND warnings /w14265)
        # 'operator': unsigned/negative constant mismatch
        list(APPEND warnings /w14287)
        # loop control variable declared in the for-loop is used outside loop scope
        list(APPEND warnings /we4289)
        # 'operator': expression is always 'boolean_value'
        list(APPEND warnings /w14296)
        # 'variable': pointer truncation from 'type1' to 'type2'
        list(APPEND warnings /w14311)
        # expression before comma evaluates to a function which is missing arguments
        list(APPEND warnings /w14545)
        # function call before comma missing argument list
        list(APPEND warnings /w14546)
        # 'operator': operator before comma has no effect
        list(APPEND warnings /w14547)
        # 'operator': operator before comma has no effect; did you intend 'operator'?
        list(APPEND warnings /w14549)
        # expression has no effect; expected expression with side- effect
        list(APPEND warnings /w14555)
        # pragma warning: there is no warning number 'number'
        list(APPEND warnings /w14619)
        # Enable warning on thread un-safe static member initialization
        list(APPEND warnings /w14640)
        # Conversion from 'type1' to 'type2' is sign-extended.
        list(APPEND warnings /w14826)
        # wide string literal cast to 'LPSTR'
        list(APPEND warnings /w14905)
        # string literal cast to 'LPWSTR'
        list(APPEND warnings /w14906)
        # illegal copy-initialization; more than one user-defined conversion
        list(APPEND warnings /w14928)


        ############################3
        
        # disable Spectre mitigation code generation warnings in external files
        # list(APPEND warnings /wd5045)
        # disable left-to-right evaluation order in braced initializer list
        # list(APPEND warnings /wd4868)
        

        # if (CMAKE_BUILD_TYPE STREQUAL "Release")
            # unreachable code warnings when using folly::small_vector in release
            # list(APPEND warnings /wd4702)
        # else()
            # disable unused parameters in debug
            # list(APPEND warnings /wd4100)
        # endif()
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

        # warn for implicit conversion loses precision
        list(APPEND warnings -Wconversion)
        # warn the user if a class with virtual functions has a non-virtual destructor.
        list(APPEND warnings -Wnon-virtual-dtor)
        # warn for c-style casts
        list(APPEND warnings -Wold-style-cast)
        # warn for potential performance problem casts
        list(APPEND warnings -Wcast-align)
        # warn on anything being unused
        list(APPEND warnings -Wunused)
        # warn if you overload (not override) a virtual function
        list(APPEND warnings -Woverloaded-virtual)
        # warn if non-standard C++ is used
        list(APPEND warnings -Wpedantic)
        # warn on type conversions that may lose data
        # list(APPEND warnings -Wconversion)
        # warn on sign conversions
        list(APPEND warnings -Wnull-dereference)
        # warn if float is implicit promoted to double
        # list(APPEND warnings -Wdouble-promotion)
        # warn on security issues around functions that format output (ie printf)
        list(APPEND warnings -Wformat=2)
        # warn on statements that fallthrough without an explicit annotation
        list(APPEND warnings -Wimplicit-fallthrough)
        
        # -Wthread-safety 
    endif()

    if ((NOT MSVC) AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # warn if indentation implies blocks where blocks do not exist
        list(APPEND warnings -Wmisleading-indentation)
        # warn if if / else chain has duplicated conditions
        list(APPEND warnings -Wduplicated-cond)
        # warn if if / else branches have duplicated code
        list(APPEND warnings -Wduplicated-branches)
        # warn about logical operations being used where bitwise were probably wanted
        list(APPEND warnings -Wlogical-op)
        # warn if you perform a cast to the same type
        # list(APPEND warnings -Wuseless-cast)
        # warn if an overridden member function is not marked 'override' or 'final'
        list(APPEND warnings -Wsuggest-override)
    endif()

    target_compile_options("${target_name}" INTERFACE ${warnings})
endfunction()


#
# Disable specific warnings that are problematic for the target.`
#
# Its a good practice to re-check them from time to time
#
function(ls_set_compiler_warnings_disabled target_name)
    set(warnings "")

    # Unreachable code warnings in MSVC release LTO builds. 
    # While they happen in headers they are not excluded with /extern:W0, as
    # they happen during code generation. Will not be fixed.
    # last-check: 2024-09-02
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND 
        (CMAKE_BUILD_TYPE STREQUAL "Release" 
         OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
        list(APPEND warnings /wd4702)
    endif()

    # g++ generates those in folly headers for release builds
    # last-check: 2024-09-02
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_BUILD_TYPE STREQUAL "Release")
        list(APPEND warnings -Wno-array-bounds)
        list(APPEND warnings -Wno-stringop-overread)
    endif()

    # g++ generates those in folly & range-v3 headers for release non-lto builds
    # last-check: 2024-09-02
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_BUILD_TYPE STREQUAL "Release")
        list(APPEND warnings -Wno-maybe-uninitialized)
        list(APPEND warnings -Wno-null-dereference)
    endif()

    # g++ generates those in folly & range-v3 headers for sanitized builds
    # last-check: 2024-09-02
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND 
        (LS_SANITIZE STREQUAL "Address;Undefined" OR LS_SANITIZE STREQUAL "Address"))
        list(APPEND warnings -Wno-maybe-uninitialized)
        list(APPEND warnings -Wno-array-bounds)
        list(APPEND warnings -Wno-stringop-overflow)
    endif()

    # g++ generates those in range-v3 headers for sanitized builds
    # last-check: 2025-12-09
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
        (LS_SANITIZE STREQUAL "Undefined" OR
         LS_SANITIZE STREQUAL "Address;Undefined" OR
         LS_SANITIZE STREQUAL "Thread"))
        list(APPEND warnings -Wno-maybe-uninitialized)
    endif()

    # clang generates those in boost headers for non-pch debug builds on linux
    # last-check: 2024-09-02
    if (NOT WIN32 AND
        CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND 
        CMAKE_BUILD_TYPE STREQUAL "Debug" AND
        NOT LS_ENABLE_PCH)
        list(APPEND warnings -Wno-deprecated-declarations)
    endif()

    target_compile_options("${target_name}" INTERFACE ${warnings})
endfunction()

