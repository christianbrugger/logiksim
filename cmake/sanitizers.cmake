
#
# Set Sanitizer flags
#
# Sanitizer flags should be added to all targets, except libc++ which
# uses cmake config to set them.
#
macro(ls_setup_sanitizers target_name sanitizer_selection)
    if (MSVC)
        ls_setup_sanitizers_msvc("${target_name}" "${sanitizer_selection}")
    else()
        ls_setup_sanitizers_gnu("${target_name}" "${sanitizer_selection}")
    endif()
endmacro()

#
# Set Sanitizer flags for compilers with GNU flags
#
function(ls_setup_sanitizers_gnu target_name sanitizer_selection)
    if (sanitizer_selection STREQUAL "")
        # nothing to do

    elseif(sanitizer_selection STREQUAL "Address")
        target_compile_options("${target_name}" INTERFACE "-fsanitize=address") 
        target_link_options("${target_name}" INTERFACE "-fsanitize=address")

    elseif(sanitizer_selection STREQUAL "Undefined")
        target_compile_options("${target_name}" INTERFACE "-fsanitize=undefined") 
        target_link_options("${target_name}" INTERFACE "-fsanitize=undefined")

    elseif(sanitizer_selection STREQUAL "Address;Undefined")
        target_compile_options(
            "${target_name}"
            INTERFACE
            "-fsanitize=address"
            "-fsanitize=undefined"
        )
        target_link_options(
            "${target_name}"
            INTERFACE
            "-fsanitize=address"
            "-fsanitize=undefined"
        )

    elseif(sanitizer_selection STREQUAL "Memory")
        target_compile_options("${target_name}" INTERFACE "-fsanitize=memory")
        target_link_options("${target_name}" INTERFACE "-fsanitize=memory")

    elseif(sanitizer_selection STREQUAL "MemoryWithOrigins")
        target_compile_options(
            "${target_name}"
            INTERFACE
            "-fsanitize=memory"
            "-fsanitize-memory-track-origins"
        ) 
        target_link_options("${target_name}"
            INTERFACE
            "-fsanitize=memory"
            "-fsanitize-memory-track-origins"
        )
        
    elseif(sanitizer_selection STREQUAL "Thread")
        target_compile_options("${target_name}" INTERFACE "-fsanitize=thread")
        target_link_options("${target_name}" INTERFACE "-fsanitize=thread")

    else()
        message(FATAL_ERROR "Unknown sanitizer_selection Option: ${sanitizer_selection}")
    endif()

    # any sanitizer
    if (sanitizer_selection)
        target_compile_options(
            "${target_name}"
            INTERFACE
            "-fno-sanitize-recover=all"
            "-fno-omit-frame-pointer" "-g"
        ) 
        target_link_options("${target_name}" INTERFACE "-fno-sanitize-recover=all")
    endif()

    # faster debugging profile
    if (sanitizer_selection AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(
            "${target_name}"
            INTERFACE
            "-O1"
            "-fno-optimize-sibling-calls"
        )
    endif()

endfunction()

#
# Set Sanitizer flags for compilers with MSVC flags
#
function(ls_setup_sanitizers_msvc target_name sanitizer_selection)
    if (sanitizer_selection STREQUAL "")
        # nothing to do

    elseif(sanitizer_selection STREQUAL "Address")
        target_compile_options("${target_name}" INTERFACE "/fsanitize=address") 
        target_link_options("${target_name}" INTERFACE "/fsanitize=address") 
        # target_link_options("${target_name}" INTERFACE "/INFERASANLIBS")
        # find_program(LS_LINK_PROGRAM link)
        # set(CMAKE_LINKER ${LS_LINK_PROGRAM} PARENT_SCOPE)
        # target_link_directories("${target_name}" INTERFACE "C:/Program Files/llvm/lib/clang/18/lib/windows")
        # target_link_libraries("${target_name}" INTERFACE
        #     "clang_rt.asan_cxx-x86_64" 
        #     "clang_rt.asan_static-x86_64"
        # )

    elseif(sanitizer_selection STREQUAL "Undefined")
        target_compile_options("${target_name}" INTERFACE "/fsanitize=undefined") 
        target_link_options("${target_name}" INTERFACE "/fsanitize=undefined")

    else()
        message(FATAL_ERROR "Unknown sanitizer_selection Option: ${sanitizer_selection}")
    endif()

    # any sanitizer
    if (sanitizer_selection)
        # sanitizers do not support the runtime debug library
        # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" PARENT_SCOPE)
    endif()



endfunction()
