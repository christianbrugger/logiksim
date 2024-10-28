include(../../cmake/utils.cmake)

include(source_list.cmake)


###### GUI Executable

add_executable(${LS_GUI_TARGET}
    ${LS_MAIN_GUI_SOURCES}
)
target_link_libraries(${LS_GUI_TARGET} PRIVATE 
    ${LS_GUI_LIBRARY}
    
)
set_property(TARGET ${LS_GUI_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${LS_GUI_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)


###### Packaging Configuration

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${LS_GUI_TARGET} PRIVATE 
        LS_EXCEPTION_MESSAGE_BOX
    )
endif ()

ls_require_bool(LS_WIN32_APPLICATON)
if (WIN32 AND LS_WIN32_APPLICATON)
    set_property(TARGET ${LS_GUI_TARGET} PROPERTY WIN32_EXECUTABLE true)
    target_compile_definitions(${LS_GUI_TARGET} PRIVATE LS_LOG_TO_FILE)
endif ()
