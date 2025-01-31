include(../cmake/utils.cmake)

# Qt is not build from source and is incompatible with clang libc++
ls_require_bool(LS_USE_LIBCXX)
if (NOT LS_USE_LIBCXX)
    set(LS_BUILD_GUI TRUE)
else()
    set(LS_BUILD_GUI FALSE)
endif()


ls_require_bool(LS_USE_LIBCXX)
if(LS_GUI_SAMPLE)
    add_subdirectory(sample_gui)

elseif(LS_SANITIZE MATCHES "Fuzzer")
    # libraries
    add_subdirectory(core)

    # fuzz
    add_subdirectory(fuzz_core)

else()
    # libraries
    add_subdirectory(core)
    add_subdirectory(core_export)

    # executables & tests
    add_subdirectory(main_benchmark)
    add_subdirectory(main_cli)
    add_subdirectory(test_core)

    # gui lib, executables & tests
    if (LS_BUILD_GUI)
        add_subdirectory(gui)
        add_subdirectory(main_gui)
        add_subdirectory(test_gui)
    endif()
endif()

