#ifndef LOGICSIM_ALGORITHM_PATH_CONVERSION_H
#define LOGICSIM_ALGORITHM_PATH_CONVERSION_H

#include <filesystem>
#include <string_view>

namespace logicsim {

[[nodiscard]] auto to_path(std::string_view filename) -> std::filesystem::path;

}

#endif
