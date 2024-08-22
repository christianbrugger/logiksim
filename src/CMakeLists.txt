# libraries
add_subdirectory(core)

# executables
add_subdirectory(benchmark)
add_subdirectory(cli)

# Memory and thread sanitizers require all dependencies to be build with santizers.
# This is not possible with the GUI, as Qt is not sanitizer clean.
if (NOT LS_SANITIZE STREQUAL "Memory" AND
    NOT LS_SANITIZE STREQUAL "MemoryWithOrigins" AND
    NOT LS_SANITIZE STREQUAL "Thread")
    add_subdirectory(gui)
endif()

