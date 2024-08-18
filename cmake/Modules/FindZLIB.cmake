

if (TARGET ZLIB::ZLIB)
    set(ZLIB_FOUND TRUE)
else()
    add_subdirectory(
        ${PROJECT_SOURCE_DIR}/external/zlib
        ${CMAKE_BINARY_DIR}/external/zlib
        EXCLUDE_FROM_ALL
        SYSTEM
    )

    add_library(ZLIB::ZLIB ALIAS zlibstatic)
    set(ZLIB_FOUND TRUE)
endif ()
