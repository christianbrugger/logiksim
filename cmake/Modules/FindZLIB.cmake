

if (TARGET ZLIB::ZLIB)
    set(ZLIB_FOUND TRUE)
else()
    add_subdirectory(external/zlib EXCLUDE_FROM_ALL SYSTEM)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
    set(ZLIB_FOUND TRUE)
endif ()
