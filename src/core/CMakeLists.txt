include(source_list.cmake)


###### Dependencies

# Zlib
set(ZLIB_USE_STATIC_LIBS ON)
find_package(ZLIB REQUIRED)


# Boost
set(BOOST_IOSTREAMS_ENABLE_ZLIB ON)
set(BOOST_INCLUDE_LIBRARIES 
    algorithm 
    container 
    geometry
    iostreams
    random 
    safe_numerics
)
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/boost
    ${CMAKE_BINARY_DIR}/external/boost
    EXCLUDE_FROM_ALL
    SYSTEM
)

# Blend2D
set(BLEND2D_STATIC TRUE)
set(BLEND2D_NO_INSTALL TRUE)
if (LS_BLEND2D_NO_JIT)
    set(BLEND2D_NO_JIT ON)
endif()
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/blend2d
    ${CMAKE_BINARY_DIR}/external/blend2d
    EXCLUDE_FROM_ALL
    SYSTEM
)

# fmt
set(FMT_INSTALL FALSE)
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/fmt
    ${CMAKE_BINARY_DIR}/external/fmt
    EXCLUDE_FROM_ALL
    SYSTEM
)


# range-v3
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/range-v3
    ${CMAKE_BINARY_DIR}/external/range-v3
    EXCLUDE_FROM_ALL
    SYSTEM
)


# GSL
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/GSL
    ${CMAKE_BINARY_DIR}/external/GSL
    EXCLUDE_FROM_ALL
    SYSTEM
)


# abseil
set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/abseil-cpp
    ${CMAKE_BINARY_DIR}/external/abseil-cpp
    EXCLUDE_FROM_ALL
    SYSTEM
)


# Folly
# Only build a minimal version of the library, as there is no CMake build script
set(FOLLY_DIR_NAME my_folly)
add_library(folly
    ${PROJECT_SOURCE_DIR}/external/${FOLLY_DIR_NAME}/folly/memory/detail/MallocImpl.cpp
    ${PROJECT_SOURCE_DIR}/external/${FOLLY_DIR_NAME}/folly/lang/SafeAssert.cpp
    ${PROJECT_SOURCE_DIR}/external/${FOLLY_DIR_NAME}/folly/ScopeGuard.cpp
    ${PROJECT_SOURCE_DIR}/external/${FOLLY_DIR_NAME}/folly/lang/ToAscii.cpp
)
target_include_directories(folly SYSTEM PUBLIC 
    ${PROJECT_SOURCE_DIR}/external/${FOLLY_DIR_NAME}
)
target_compile_definitions(folly PUBLIC FOLLY_NO_CONFIG)


# unordered_dense (ankerl)
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/unordered_dense
    ${CMAKE_BINARY_DIR}/external/unordered_dense
    EXCLUDE_FROM_ALL
    SYSTEM
)


# glaze
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/glaze
    ${CMAKE_BINARY_DIR}/external/glaze
    EXCLUDE_FROM_ALL
    SYSTEM
)


# cppcodec / base64
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/cppcodec
    ${CMAKE_BINARY_DIR}/external/cppcodec
    EXCLUDE_FROM_ALL
    SYSTEM
)


# harfbuzz
set(SKIP_INSTALL_ALL TRUE)
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/harfbuzz
    ${CMAKE_BINARY_DIR}/external/harfbuzz
    EXCLUDE_FROM_ALL
    SYSTEM
)


# GCE-Math
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/gcem
    ${CMAKE_BINARY_DIR}/external/gcem
    EXCLUDE_FROM_ALL
    SYSTEM
)


# svg2b2d
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/svg2b2d
    ${CMAKE_BINARY_DIR}/external/svg2b2d
    EXCLUDE_FROM_ALL
    SYSTEM
)


# expected
set(EXPECTED_BUILD_TESTS OFF)
add_subdirectory(
    ${PROJECT_SOURCE_DIR}/external/expected
    ${CMAKE_BINARY_DIR}/external/expected
    EXCLUDE_FROM_ALL
    SYSTEM
)


# whereami
# has no cmake build script
add_library(whereami STATIC EXCLUDE_FROM_ALL
    ${PROJECT_SOURCE_DIR}/external/whereami/src/whereami.c
)
set_source_files_properties(
    ${PROJECT_SOURCE_DIR}/external/whereami/src/whereami.c
    PROPERTIES LANGUAGE CXX
)
target_include_directories(whereami PUBLIC
    ${PROJECT_SOURCE_DIR}/external/whereami/src
)


###### Library Include


set(LS_CORE_LIBRARIES
    Boost::algorithm
    Boost::container
    Boost::geometry
    Boost::iostreams
    Boost::random
	Boost::safe_numerics
    
    cppcodec
    fmt::fmt
    folly
    absl::base
    absl::flat_hash_map
    unordered_dense::unordered_dense
    GSL
    range-v3
    Blend2D::Blend2D
    glaze::glaze
    ZLIB::ZLIB
    harfbuzz
    gcem
    svg2b2d
    tl::expected
    whereami
)

###### PCH Setup

add_library(logiksim_pch INTERFACE)
if (LS_ENABLE_PCH)
    message(NOTICE "LOGIKSIM: Enabling pre-compiled headers.")

    target_precompile_headers(logiksim_pch INTERFACE
        <ankerl/unordered_dense.h>
        <blend2d.h>
        <boost/algorithm/string/join.hpp>
        <boost/container/small_vector.hpp>
        <boost/container/vector.hpp>
        <boost/geometry.hpp>
        <boost/random/mersenne_twister.hpp>
        <boost/random/uniform_int_distribution.hpp>
        <boost/safe_numerics/safe_integer.hpp>
        <fmt/core.h>
        <fmt/format.h>
        <folly/small_vector.h>
        <gcem.hpp>
        <gsl/gsl>
    
        <algorithm>
        <chrono>
        <cmath>
        <concepts>
        <cstdint>
        <exception>
        <filesystem>
        <functional>
        <iterator>
        <locale>
        <optional>
        <ostream>
        <queue>
        <random>
        <ranges>
        <ratio>
        <string>
        <string_view>
        <type_traits>
        <utility>
        <variant>
        <vector>
    )

    # this is needed so ccache works with PCH files, see:
    # https://ccache.dev/manual/latest.html#_precompiled_headers
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(logiksim_pch INTERFACE
            -Xclang -fno-pch-timestamp
        )
    endif()
else()
    message(NOTICE "LOGIKSIM: Disabling pre-compiled headers.")
endif()


###### Resources

# TODO: move to function / other CMakeLists.txt file ???

# Resources
file(GLOB_RECURSE
    LS_RESOURCE_FILES
    RELATIVE ${PROJECT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${PROJECT_SOURCE_DIR}/resources/**.ttf
    ${PROJECT_SOURCE_DIR}/resources/**.svg
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


###### Core Library


set(MAIN_LIBRARY "simulation_srp")

add_library(${MAIN_LIBRARY} 
    ${LS_CORE_SOURCES}
    ${LS_RESOURCE_FILES}
)
# TODO: remove this and add prefix core to all includes?
target_include_directories(${MAIN_LIBRARY} PUBLIC
    ${PROJECT_SOURCE_DIR}/src/core
)
target_include_directories(${MAIN_LIBRARY} PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(${MAIN_LIBRARY} PUBLIC 
    ${LS_CORE_LIBRARIES}
)
target_link_libraries(${MAIN_LIBRARY} PUBLIC 
    logiksim_options
)
target_link_libraries(${MAIN_LIBRARY} PRIVATE 
    logiksim_pch
)

