include(../cmake/utils.cmake)

# libraries
add_subdirectory(core)
add_subdirectory(test_core)

# executables
add_subdirectory(benchmark)
add_subdirectory(cli)

# Qt is not build from source and is incompatible with clang libc++
ls_require_bool(LS_USE_LIBCXX)
if (NOT LS_USE_LIBCXX)
    add_subdirectory(gui)
    add_subdirectory(test_gui)
endif()

