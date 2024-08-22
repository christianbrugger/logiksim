
function(ls_print_compiler_and_flags)
    include(CMakePrintHelpers)
    cmake_print_variables(CMAKE_C_COMPILER)
    cmake_print_variables(CMAKE_CXX_COMPILER)
    cmake_print_variables(CMAKE_CXX_COMPILER_LAUNCHER)

    cmake_print_variables(CMAKE_C_FLAGS_DEBUG)
    cmake_print_variables(CMAKE_CXX_FLAGS_DEBUG)
    cmake_print_variables(CMAKE_C_FLAGS_RELWITHDEBINFO)
    cmake_print_variables(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    cmake_print_variables(CMAKE_C_FLAGS_RELEASE)
    cmake_print_variables(CMAKE_CXX_FLAGS_RELEASE)

endfunction()


function(ls_print_project_options)
    include(CMakePrintHelpers)

    cmake_print_variables(LS_ENABLE_TIME_TRACE)
    cmake_print_variables(LS_ENABLE_CCACHE)
    cmake_print_variables(LS_ENABLE_LTO)
    cmake_print_variables(LS_ENABLE_PCH)
    cmake_print_variables(LS_ENABLE_COVERAGE)

    cmake_print_variables(LS_BLEND2D_NO_JIT)
    cmake_print_variables(LS_WARNINGS_AS_ERRORS)

    cmake_print_variables(LS_SANITIZE)
    cmake_print_variables(LS_USE_LIBCXX)
endfunction()

