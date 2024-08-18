
set(MAIN_CLI_TARGET "simulation_srp_cli")
add_executable(${MAIN_CLI_TARGET}
    main_cli.cpp
)
target_link_libraries(${MAIN_CLI_TARGET} PRIVATE 
    ${MAIN_LIBRARY} 
)
set_property(TARGET ${MAIN_CLI_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${MAIN_CLI_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

