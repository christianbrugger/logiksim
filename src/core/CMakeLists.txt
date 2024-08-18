include(source_list.cmake)


###### Dependencies


###### Core Library

set(MAIN_LIBRARY "simulation_srp")

add_library(${MAIN_LIBRARY} 
    ${LS_CORE_SOURCES}
    ${LS_RESOURCE_FILES}
)
# TODO: remove this and add prefix gui to all includes?
target_include_directories(${MAIN_LIBRARY} PUBLIC
    ${PROJECT_SOURCE_DIR}/src/core
)
target_include_directories(${MAIN_LIBRARY} PUBLIC
    ${LS_CORE_INCLUDE_DIRS}
)
target_link_libraries(${MAIN_LIBRARY} PUBLIC 
    ${LS_CORE_LIBRARIES}
)
target_link_libraries(${MAIN_LIBRARY} PRIVATE 
    logicsim_pch
)
target_compile_options(${MAIN_LIBRARY} PUBLIC ${MAIN_COMPILE_OPTIONS})

