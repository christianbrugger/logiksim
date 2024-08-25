
function(ls_setup_standard_library_static target_name use_libcxx sanitizers)
    include(cmake/utils.cmake)
    ls_require_bool(use_libcxx)

    if (use_libcxx)
        message(NOTICE "LOGIKSIM: Using static clang libc++.")

        set(LLVM_ENABLE_RUNTIMES "libcxx;libcxxabi;libunwind" CACHE STRING "" FORCE)

        set(LIBCXX_ENABLE_SHARED OFF)
        set(LIBCXXABI_ENABLE_SHARED OFF)
        set(LIBCXX_ENABLE_STATIC ON)
        set(LIBCXXABI_ENABLE_STATIC ON)

        set(LLVM_USE_SANITIZER "${sanitizers}")
        add_subdirectory(external/llvm-project/runtimes EXCLUDE_FROM_ALL SYSTEM)
        target_link_libraries(
            "${target_name}"
            INTERFACE
            cxx_static
            cxxabi_static
        )

        # The google benchmark contains an unconditional export that requires
        # all its dependencies to be exportable. By itself llvm does not define
        # export for runtime targets. Thats why we define them here.
        export(TARGETS 
            cxx_static cxxabi_static cxx-headers cxxabi_static_objects
            libcxx-abi-headers cxxabi-headers 
            FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/LlvmRuntimeTargets.cmake"
        )

    else()
        message(NOTICE "LOGIKSIM: Using default stdlib.")
    endif()

endfunction()


function(ls_setup_standard_library_shared target_name use_libcxx sanitizers)
    include(cmake/utils.cmake)
    ls_require_bool(use_libcxx)

    if (use_libcxx)
        message(NOTICE "LOGIKSIM: Using shared clang libc++.")

        set(LLVM_ENABLE_RUNTIMES "libcxx;libcxxabi;libunwind" CACHE STRING "" FORCE)

        set(LIBCXX_ENABLE_SHARED ON)
        set(LIBCXXABI_ENABLE_SHARED ON)
        set(LIBCXX_ENABLE_STATIC OFF)
        set(LIBCXXABI_ENABLE_STATIC OFF)

        set(LLVM_USE_SANITIZER "${sanitizers}")
        add_subdirectory(external/llvm-project/runtimes EXCLUDE_FROM_ALL SYSTEM)
        target_link_libraries(
            "${target_name}"
            INTERFACE
            cxx_shared
            cxxabi_shared
        )

        # The google benchmark contains an unconditional export that requires
        # all its dependencies to be exportable. By itself llvm does not define
        # export for runtime targets. Thats why we define them here.
        export(TARGETS 
            cxx_shared cxxabi_shared cxx-headers cxxabi_shared_objects 
            libcxx-abi-headers cxxabi-headers 
            FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/LlvmRuntimeTargets.cmake"
        )

    else()
        message(NOTICE "LOGIKSIM: Using default stdlib.")
    endif()

endfunction()

