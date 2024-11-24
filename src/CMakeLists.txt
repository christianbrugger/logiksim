include(../cmake/utils.cmake)

# Qt is not build from source and is incompatible with clang libc++
ls_require_bool(LS_USE_LIBCXX)
if (NOT LS_USE_LIBCXX)
    set(LS_BUILD_GUI TRUE)
else()
    set(LS_BUILD_GUI FALSE)
endif()

# libraries
add_subdirectory(core)


if(LS_SANITIZE MATCHES "Fuzzer")
    # fuzz
    add_subdirectory(fuzz_core)
else()
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

