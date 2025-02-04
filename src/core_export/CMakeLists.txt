
set(LS_CORE_EXPORT_SOURCES
    logicsim_core_export.cpp
)

###### Core Shared Library

add_library(${LS_CORE_SHARED} SHARED
    ${LS_CORE_EXPORT_SOURCES}
)
target_link_libraries(${LS_CORE_SHARED} PRIVATE 
    ${LS_CORE_LIBRARY}
)
target_compile_definitions(${LS_CORE_SHARED} PRIVATE
    LS_CORE_LIB_BUILD_SHARED
)

# https://cmake.org/cmake/help/latest/module/GenerateExportHeader.html

# set_property(TARGET ${LS_CORE_SHARED} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
#     ${CMAKE_BINARY_DIR}
# )
# set_property(TARGET ${LS_CORE_SHARED} PROPERTY RUNTIME_OUTPUT_NAME 
#     "logicsim_core"
# )

# Check symbols on Windows via:
#   dumpbin /EXPORTS .\src\core_export\ls_core_shared.dll

####### Static Export Library


add_library(${LS_CORE_STATIC} STATIC
    ${LS_CORE_EXPORT_SOURCES}
)
target_link_libraries(${LS_CORE_STATIC} PUBLIC 
    ${LS_CORE_LIBRARY}
)
target_compile_definitions(${LS_CORE_STATIC} PUBLIC
    LS_CORE_LIB_BUILD_STATIC
)


####### Verify header is C importable

# add a c++ header to the project, so the c++ linker is used,
# this is required for ubsan build on linux requiring clang++, not clang linker.
set_source_files_properties(logicsim_core_export.h PROPERTIES LANGUAGE CXX)
set_source_files_properties(main_verify.c PROPERTIES LANGUAGE C)

add_executable(${LS_VERIFY_CORE_SHARED_C}
    main_verify.c
    logicsim_core_export.h
)
set_property(TARGET ${LS_VERIFY_CORE_SHARED_C} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
target_link_libraries(${LS_VERIFY_CORE_SHARED_C} PRIVATE 
    ${LS_CORE_SHARED}
)

# while cmake has no way to limit the maximum standard, at least on linux
# it passes c99 to clang to make sure our header is C99 compatible.
set_property(TARGET ${LS_VERIFY_CORE_SHARED_C} PROPERTY C_STANDARD 99)
set_property(TARGET ${LS_VERIFY_CORE_SHARED_C} PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET ${LS_VERIFY_CORE_SHARED_C} PROPERTY C_EXTENSIONS OFF)

####### Verify header is C++ importable

set_source_files_properties(main_verify.cpp PROPERTIES LANGUAGE CXX)

add_executable(${LS_VERIFY_CORE_SHARED_CPP}
    main_verify.cpp
)
set_property(TARGET ${LS_VERIFY_CORE_SHARED_CPP} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)
target_link_libraries(${LS_VERIFY_CORE_SHARED_CPP} PRIVATE 
    ${LS_CORE_SHARED}
)
