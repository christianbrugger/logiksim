include(source_list.cmake)


###### Core Shared Library

add_library(${LS_CORE_SHARED} SHARED
    ${LS_CORE_SHARED_SOURCES}
)
target_compile_definitions(${LS_CORE_SHARED} PRIVATE
    LS_CORE_LIB_BUILD_SHARED
)
target_link_libraries(${LS_CORE_SHARED} PRIVATE 
    ${LS_CORE_LIBRARY}
)
set_property(TARGET ${LS_CORE_SHARED} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${LS_CORE_SHARED} PROPERTY RUNTIME_OUTPUT_NAME 
    "logicsim_core"
)
set_property(TARGET ${LS_CORE_SHARED} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

