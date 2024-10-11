#ifndef LOGICSIM_CORE_ALGORITHM_TO_PATH_H
#define LOGICSIM_CORE_ALGORITHM_TO_PATH_H

#include <filesystem>
#include <string_view>

namespace logicsim {

/**
 * @brief: Construct utf-8 path from a string-view containing utf-8 data.
 */
[[nodiscard]] auto to_path(std::string_view text) -> std::filesystem::path;

}  // namespace logicsim

#endif
