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


###### GUI Executable

set(MAIN_GUI_TARGET "simulation_srp_gui")

add_executable(${MAIN_GUI_TARGET}
    ${LS_GUI_SOURCES}
    ${MOC_SOURCES}
)
# TODO: remove this and add prefix gui to all includes?
target_include_directories(${MAIN_LIBRARY} PUBLIC
    ${PROJECT_SOURCE_DIR}/src/gui
)
target_link_libraries(${MAIN_GUI_TARGET} PRIVATE 
    ${MAIN_LIBRARY}
    
    Qt6::Core
    Qt6::Widgets
    Qt6::GuiPrivate
)
set_property(TARGET ${MAIN_GUI_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${MAIN_GUI_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)


###### Packaging Configuration

# TODO: introduce packaging flag
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${MAIN_GUI_TARGET} PRIVATE 
        LS_EXCEPTION_MESSAGE_BOX
    )
    if (WIN32)
        # set_property(TARGET ${MAIN_GUI_TARGET} PROPERTY WIN32_EXECUTABLE true)
        # target_compile_definitions(${MAIN_GUI_TARGET} PRIVATE LS_LOG_TO_FILE)
    endif ()
endif ()
