
# Resources

file(GLOB_RECURSE
    LS_RESOURCE_FILES
    RELATIVE ${PROJECT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${PROJECT_SOURCE_DIR}/resources/fonts/lucide_*_300.ttf
    ${PROJECT_SOURCE_DIR}/resources/fonts/NotoSans*.ttf
    ${PROJECT_SOURCE_DIR}/resources/fonts/LICENSE*
    
    ${PROJECT_SOURCE_DIR}/resources/icons/lucide/*.svg
    ${PROJECT_SOURCE_DIR}/resources/icons/lucide/LICENSE
    ${PROJECT_SOURCE_DIR}/resources/icons/derivative/*.svg
    ${PROJECT_SOURCE_DIR}/resources/icons/derivative/*.ico
    ${PROJECT_SOURCE_DIR}/resources/icons/derivative/LICENSE
)
FOREACH(LS_RESOURCE_FILE ${LS_RESOURCE_FILES})
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/${LS_RESOURCE_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${PROJECT_SOURCE_DIR}/${LS_RESOURCE_FILE}
            ${CMAKE_BINARY_DIR}/${LS_RESOURCE_FILE}
        DEPENDS ${PROJECT_SOURCE_DIR}/${LS_RESOURCE_FILE}
  )
ENDFOREACH()
list(TRANSFORM LS_RESOURCE_FILES PREPEND ${CMAKE_BINARY_DIR}/)

add_library(logiksim_resource_files INTERFACE
    ${LS_RESOURCE_FILES}
)

