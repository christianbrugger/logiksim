include(source_list.cmake)

###### Dependencies

# Catch2
set(CATCH_INSTALL_DOCS OFF)
set(CATCH_INSTALL_EXTRA OFF)
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/catch2
    ${CMAKE_BINARY_DIR}/external/catch2
    EXCLUDE_FROM_ALL
    SYSTEM
)


###### Test Executable

add_executable(${LS_GUI_TEST_TARGET}
    ${LS_GUI_TEST_SOURCES}
)
target_link_libraries(${LS_GUI_TEST_TARGET} PRIVATE 
    ${LS_GUI_LIBRARY} 
    Catch2::Catch2WithMain
)
set_property(TARGET ${LS_GUI_TEST_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${LS_GUI_TEST_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)


###### Coverage for Tests

# TODO: move to function and cmake folder
if (LS_ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")  #  AND CMAKE_BUILD_TYPE MATCHES Debug
        message(NOTICE "LOGIKSIM: Enabling Clang Coverage for Tests.")
        target_compile_options(${LS_GUI_TEST_TARGET} PRIVATE 
            -fprofile-instr-generate 
            -fcoverage-mapping
        )
        target_link_options(${LS_GUI_TEST_TARGET} PRIVATE 
            -fprofile-instr-generate 
            -fcoverage-mapping
        )
    else()
        message(FATAL_ERROR "Coverage only works for Clang!" )
    endif()
endif()


###### CMake Test Discovery

enable_testing()
include(CTest)
include(Catch)
catch_discover_tests(${LS_GUI_TEST_TARGET})


