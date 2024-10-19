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

#
# Deprecations
#

# This disables all deprecated features before the minimum supported version.
# This usually is the system installed Qt version on latest Ubuntu LTS.
target_compile_definitions(${LS_GUI_LIBRARY} PUBLIC
    QT_DISABLE_DEPRECATED_BEFORE=0x060400  # Qt 6.4
)

# Disable deprecation warnings introduced in newer version, e.g. 6.8.
# This is because the can't, due to depency on older version and replacements
# not being available there.
if (MSVC)
    target_compile_options(${LS_GUI_LIBRARY} PRIVATE
        /wd4996  # deprecated
    )
else()
    target_compile_options(${LS_GUI_LIBRARY} PRIVATE
        -Wno-deprecated-declarations
    )
endif()
