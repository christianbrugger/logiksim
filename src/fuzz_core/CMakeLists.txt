

# Executable


add_executable(${LS_FUZZ_CORE}
    main.cpp
)
target_include_directories(${LS_FUZZ_CORE} PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(${LS_FUZZ_CORE} PRIVATE 
    ${LS_CORE_LIBRARY} 
)
set_property(TARGET ${LS_FUZZ_CORE} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${LS_FUZZ_CORE} PROPERTY POSITION_INDEPENDENT_CODE TRUE)


