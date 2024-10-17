include(source_list.cmake)
include(source_moc.cmake)


###### Dependencies

# Qt 6
set(CMAKE_AUTOMOC FALSE)  # takes too much time, not parallelized
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
message(NOTICE "Qt version: ${Qt6_VERSION}")


###### MOC Files

# list MOC files manually to avoid slow glob
QT6_WRAP_CPP(MOC_SOURCES ${LS_MOC_FILES})

###### GUI Library

add_library(${LS_GUI_LIBRARY} 
    ${LS_GUI_SOURCES}
    ${MOC_SOURCES}
)
target_link_libraries(${LS_GUI_LIBRARY} PUBLIC 
    ${LS_CORE_LIBRARY}

    Qt6::Core
    Qt6::Widgets
    Qt6::GuiPrivate
)

###### GUI Executable

add_executable(${LS_GUI_TARGET}
    main_gui.cpp
)
target_link_libraries(${LS_GUI_TARGET} PRIVATE 
    ${LS_GUI_LIBRARY}
    
)
set_property(TARGET ${LS_GUI_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${LS_GUI_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)


###### Packaging Configuration

# TODO: introduce packaging flag
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${LS_GUI_TARGET} PRIVATE 
        LS_EXCEPTION_MESSAGE_BOX
    )
    if (WIN32)
        # set_property(TARGET ${LS_GUI_TARGET} PROPERTY WIN32_EXECUTABLE true)
        # target_compile_definitions(${LS_GUI_TARGET} PRIVATE LS_LOG_TO_FILE)
    endif ()
endif ()
