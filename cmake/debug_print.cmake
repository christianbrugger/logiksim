
function(ls_print_compiler_and_flags)
    include(CMakePrintHelpers)
    cmake_print_variables(CMAKE_C_COMPILER)
    cmake_print_variables(CMAKE_CXX_COMPILER)

    cmake_print_variables(CMAKE_C_FLAGS_DEBUG)
    cmake_print_variables(CMAKE_CXX_FLAGS_DEBUG)
    cmake_print_variables(CMAKE_C_FLAGS_RELWITHDEBINFO)
    cmake_print_variables(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    cmake_print_variables(CMAKE_C_FLAGS_RELEASE)
    cmake_print_variables(CMAKE_CXX_FLAGS_RELEASE)

endfunction()

