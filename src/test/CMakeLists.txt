include(source_list.cmake)

###### Dependencies

# Google Test
set(INSTALL_GTEST OFF)
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/googletest
    ${CMAKE_BINARY_DIR}/external/googletest
    EXCLUDE_FROM_ALL
    SYSTEM
)


###### Test Executable

set(MAIN_TEST_TARGET "simulation_srp_test")

add_executable(${MAIN_TEST_TARGET}
    ${LS_TEST_SOURCES}
)
target_link_libraries(${MAIN_TEST_TARGET} PRIVATE 
    ${MAIN_LIBRARY} 
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
)
target_include_directories(${MAIN_TEST_TARGET} PUBLIC
    ${PROJECT_SOURCE_DIR}
)
set_property(TARGET ${MAIN_TEST_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${MAIN_TEST_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)


###### Coverage for Tests

# TODO: move to function and cmake folder
if (LS_ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")  #  AND CMAKE_BUILD_TYPE MATCHES Debug
        message(NOTICE "LOGIKSIM: Enabling Clang Coverage for Tests.")
        target_compile_options(${MAIN_TEST_TARGET} PRIVATE 
            -fprofile-instr-generate 
            -fcoverage-mapping
        )
        target_link_options(${MAIN_TEST_TARGET} PRIVATE 
            -fprofile-instr-generate 
            -fcoverage-mapping
        )
    else()
        message(FATAL_ERROR "Coverage only works for Clang!" )
    endif()
endif()


###### CMake Test Discovery

enable_testing()
include(GoogleTest)
gtest_discover_tests(${MAIN_TEST_TARGET})


