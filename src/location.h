#ifndef LOGICSIM_LOCATION_H
#define LOGICSIM_LOCATION_H

#include <filesystem>

namespace logicsim {

[[nodiscard]] auto get_executable_path() -> std::filesystem::path;

}

#endif
