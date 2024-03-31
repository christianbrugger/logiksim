#ifndef LOGICSIM_EXECUTABLE_PATH_H
#define LOGICSIM_EXECUTABLE_PATH_H

#include <filesystem>

namespace logicsim {

[[nodiscard]] auto get_executable_path() -> std::filesystem::path;

}

#endif
