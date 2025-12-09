

# Adds compile flags of the targets to future 'check_cxx_source_compiles' checks.
#
# Iterates the lsit of targets, collects their flags and adds them to the
# CMAKE_REQUIRED_* variables. Those are then used to check compiler features.
function(ls_setup_cmake_required_from_libs lib_targets)
    set(INCLUDES)
    set(OPTIONS)
    set(DEFINITIONS)

    foreach(target IN LISTS lib_targets)
        get_target_property(RESULT "${target}" INTERFACE_INCLUDE_DIRECTORIES)
        if (RESULT) 
            list(APPEND INCLUDES "${RESULT}")
        endif()

        get_target_property(RESULT "${target}" INTERFACE_COMPILE_OPTIONS)
        if (RESULT) 
            list(APPEND OPTIONS "${RESULT}")
        endif()

        get_target_property(RESULT "${target}" INTERFACE_COMPILE_DEFINITIONS)
        if (RESULT) 
            list(APPEND DEFINITIONS "${RESULT}")
        endif()
    endforeach()

    list(TRANSFORM DEFINITIONS PREPEND "-D")

    list(REMOVE_DUPLICATES INCLUDES)
    list(REMOVE_DUPLICATES OPTIONS)
    list(REMOVE_DUPLICATES DEFINITIONS)

    set(CMAKE_REQUIRED_FLAGS "${OPTIONS}" CACHE STRING "" FORCE)
    set(CMAKE_REQUIRED_INCLUDES "${INCLUDES}" CACHE STRING "" FORCE)
    set(CMAKE_REQUIRED_DEFINITIONS "${DEFINITIONS}" CACHE STRING "" FORCE)
endfunction()


function(ls_setup_standard_library_shared target_name use_libcxx sanitizers)
    include(cmake/utils.cmake)
    ls_require_bool(use_libcxx)

    if (use_libcxx)
        message(NOTICE "LOGIKSIM: Using shared clang libc++.")

        set(LLVM_ENABLE_RUNTIMES "libcxx;libcxxabi" CACHE STRING "" FORCE)

        set(LIBCXX_ENABLE_SHARED ON)
        set(LIBCXXABI_ENABLE_SHARED ON)
        set(LIBCXX_ENABLE_STATIC OFF)
        set(LIBCXXABI_ENABLE_STATIC OFF)

        set(LIBCXXABI_USE_LLVM_UNWINDER OFF)
        set(LLVM_USE_SANITIZER "${sanitizers}")

        set(LIBCXX_INCLUDE_TESTS OFF)
        set(LLVM_INCLUDE_TESTS OFF)
        set(LLVM_INCLUDE_DOCS OFF)

        add_subdirectory(external/llvm-project/runtimes EXCLUDE_FROM_ALL SYSTEM)
        target_link_libraries(
            "${target_name}"
            INTERFACE
            cxx_shared
            cxxabi_shared
        )

        # manually collect all targets
        set(ALL_LLVM_LIBS
            cxx_shared
            cxxabi_shared
            cxx-headers
            cxxabi_shared_objects
            libcxx-abi-headers
            cxxabi-headers

            libcxx-libc-shared # runtimes-libc-shared
            libcxx-abi-shared
            libcxx-libc-headers # runtimes-libc-headers
            cxxabi-reexports
        )

        # add flags to compiler checks so it is evaulated correctly
        ls_setup_cmake_required_from_libs("${ALL_LLVM_LIBS}")

        # The google benchmark contains an unconditional export that requires
        # all its dependencies to be exportable. By itself llvm does not define
        # export for runtime targets. Thats why we define them here.
        export(TARGETS 
            ${ALL_LLVM_LIBS}
            FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/LlvmRuntimeTargets.cmake"
        )

    else()
        message(NOTICE "LOGIKSIM: Using default stdlib.")
    endif()

endfunction()


function(ls_setup_standard_library_static target_name use_libcxx sanitizers)
    include(cmake/utils.cmake)
    ls_require_bool(use_libcxx)

    if (use_libcxx)
        message(NOTICE "LOGIKSIM: Using static clang libc++.")

        set(LLVM_ENABLE_RUNTIMES "libcxx;libcxxabi" CACHE STRING "" FORCE)

        set(LIBCXX_ENABLE_SHARED OFF)
        set(LIBCXXABI_ENABLE_SHARED OFF)
        set(LIBCXX_ENABLE_STATIC ON)
        set(LIBCXXABI_ENABLE_STATIC ON)

        set(LIBCXXABI_USE_LLVM_UNWINDER OFF)
        set(LLVM_USE_SANITIZER "${sanitizers}")

        add_subdirectory(external/llvm-project/runtimes EXCLUDE_FROM_ALL SYSTEM)
        target_link_libraries(
            "${target_name}"
            INTERFACE
            cxx_static
            cxxabi_static
        )

        # manually collect all targets
        set(ALL_LLVM_LIBS
            cxx_static
            cxxabi_static
            cxx-headers
            cxxabi_static_objects
            libcxx-abi-headers
            cxxabi-headers
            cxx-sanitizer-flags
        )

        # add flags to compiler checks so it is evaulated correctly
        ls_setup_cmake_required_from_libs("${ALL_LLVM_LIBS}")

        # The google benchmark contains an unconditional export that requires
        # all its dependencies to be exportable. By itself llvm does not define
        # export for runtime targets. Thats why we define them here.
        export(TARGETS 
            ${ALL_LLVM_LIBS}
            FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/LlvmRuntimeTargets.cmake"
        )

    else()
        message(NOTICE "LOGIKSIM: Using default stdlib.")
    endif()

endfunction()
