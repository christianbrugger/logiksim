
# TODO: RENAME
# set(MAIN_LIBRARY "simulation_srp")


# TODO: RENAME
set(MAIN_TARGET "simulation_srp_main")
add_executable(${MAIN_TARGET}
    main.cpp
)
target_link_libraries(${MAIN_TARGET} PRIVATE 
    ${MAIN_LIBRARY} 
)
set_property(TARGET ${MAIN_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

