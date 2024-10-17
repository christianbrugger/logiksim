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

