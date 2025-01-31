include(source_list.cmake)


###### Core Shared Library

add_library(${LS_CORE_SHARED} SHARED
    ${LS_CORE_SHARED_SOURCES}
)
target_link_libraries(${LS_CORE_SHARED} PRIVATE 
    ${LS_CORE_LIBRARY}
)
target_compile_definitions(${LS_CORE_SHARED} PRIVATE
    LS_CORE_LIB_BUILD_SHARED
)
set_property(TARGET ${LS_CORE_SHARED} PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET ${LS_CORE_SHARED} PROPERTY VISIBILITY_INLINES_HIDDEN 1)
set_property(TARGET ${LS_CORE_SHARED} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

# https://cmake.org/cmake/help/latest/module/GenerateExportHeader.html

# set_property(TARGET ${LS_CORE_SHARED} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
#     ${CMAKE_BINARY_DIR}
# )
# set_property(TARGET ${LS_CORE_SHARED} PROPERTY RUNTIME_OUTPUT_NAME 
#     "logicsim_core"
# )

# Check symbols on Windows via:
#   dumpbin /EXPORTS .\src\core_shared\ls_core_shared.dll

####### Verify header is C

set(LS_CORE_SHARED_VERIFY "ls_core_shared_verify")

set_source_files_properties(main_verify.c PROPERTIES LANGUAGE C)

add_executable(${LS_CORE_SHARED_VERIFY}
    main_verify.c
)
set_property(TARGET ${LS_CORE_SHARED_VERIFY} PROPERTY RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}
)

