
add_executable(${LS_CLI_TARGET}
    main_cli.cpp
)
target_link_libraries(${LS_CLI_TARGET} PRIVATE 
    ${LS_CORE_LIBRARY} 
)
set_property(TARGET ${LS_CLI_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${LS_CLI_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

