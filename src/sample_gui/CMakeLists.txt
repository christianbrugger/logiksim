include(../../cmake/utils.cmake)


# Qt 6
set(CMAKE_AUTOMOC FALSE)  # takes too much time, not parallelized
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Svg)
message(NOTICE "Qt version: ${Qt6_VERSION}")


###### GUI Executable

add_executable(${LS_SAMPLE_GUI_TARGET}
    main_sample_gui.cpp
)
target_link_libraries(${LS_SAMPLE_GUI_TARGET} PRIVATE 
    Qt6::Core
    Qt6::Widgets
    Qt6::Svg

    Qt6::GuiPrivate
)
set_property(TARGET ${LS_SAMPLE_GUI_TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
set_property(TARGET ${LS_SAMPLE_GUI_TARGET} PROPERTY POSITION_INDEPENDENT_CODE TRUE)


###### Packaging Configuration

ls_require_bool(LS_WIN32_APPLICATON)
if (WIN32 AND LS_WIN32_APPLICATON)
    set_property(TARGET ${LS_SAMPLE_GUI_TARGET} PROPERTY WIN32_EXECUTABLE true)
endif ()

#
# Deprecations
#

# This disables all deprecated features before the minimum supported version.
# This usually is the system installed Qt version on latest Ubuntu LTS.
target_compile_definitions(${LS_SAMPLE_GUI_TARGET} PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060400  # Qt 6.4
)

# Disable deprecation warnings introduced in newer version, e.g. 6.8.
# This is because the can't, due to depency on older version and replacements
# not being available there.
if (MSVC)
    target_compile_options(${LS_SAMPLE_GUI_TARGET} PRIVATE
        /wd4996  # deprecated
    )
else()
    target_compile_options(${LS_SAMPLE_GUI_TARGET} PRIVATE
        -Wno-deprecated-declarations
    )
endif()

