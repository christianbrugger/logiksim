

function(ls_setup_sanitizers target_name sanitizer_selection)
    # TODO -fsanitize=float-divide-by-zero

    # Set Sanitizer flags
    #
    # Set these flags after adding clang libc++. For those we pass
    # our sanitizer config and the generic settings don't work.
    #

    # TODO: make sure clang libc++ is not loaded !!!

    if (sanitizer_selection STREQUAL "")
        # nothing to do
    elseif(sanitizer_selection STREQUAL "Address")
        add_compile_options(-fsanitize=address) 
        add_link_options(-fsanitize=address)
    elseif(sanitizer_selection STREQUAL "Undefined")
        add_compile_options(-fsanitize=undefined) 
        add_link_options(-fsanitize=undefined)
    elseif(sanitizer_selection STREQUAL "Address;Undefined")
        add_compile_options(-fsanitize=address -fsanitize=undefined) 
        add_link_options(-fsanitize=address -fsanitize=undefined)
    elseif(sanitizer_selection STREQUAL "Memory")
        add_compile_options(-fsanitize=memory)
        add_link_options(-fsanitize=memory)
    elseif(sanitizer_selection STREQUAL "MemoryWithOrigins")
        add_compile_options(-fsanitize=memory -fsanitize-memory-track-origins) 
        add_link_options(-fsanitize=memory -fsanitize-memory-track-origins)
    elseif(sanitizer_selection STREQUAL "Thread")
        add_compile_options(-fsanitize=thread)
        add_link_options(-fsanitize=thread)
    else()
        message(FATAL_ERROR "Unknown sanitizer_selection Option: ${sanitizer_selection}")
    endif()

    if (sanitizer_selection)
        add_compile_options(-fno-sanitize-recover=all -fno-omit-frame-pointer -g) 
        add_link_options(-fno-sanitize-recover=all)

    endif()

    # better debugging profile
    if (sanitizer_selection AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        if (MSVC)
            add_compile_options(/O1)
        else()
            add_compile_options(-O1 -fno-optimize-sibling-calls)
        endif()
    endif()

endfunction()
