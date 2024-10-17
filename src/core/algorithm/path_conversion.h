#ifndef LOGICSIM_CORE_ALGORITHM_PATH_CONVERSION_H
#define LOGICSIM_CORE_ALGORITHM_PATH_CONVERSION_H

#include <filesystem>
#include <optional>
#include <string>

namespace logicsim {

/**
 * @brief: Tries to convert the path to utf-8 string.
 *
 * On windows paths are stored as wchar and need to be converted to utf-8.
 * This can fail if the wchar contains unpaired surrogate pairs and throws.
 *
 * Note on Linux this function can return invalid utf8.
 */
[[nodiscard]] auto path_to_utf8(const std::filesystem::path &path)
    -> std::optional<std::string>;

/**
 * @brief: Tries to convert the path to utf-8 or escapes non-asci instead.
 *
 * Note on Linux this function can return invalid utf8.
 */
[[nodiscard]] auto path_to_utf8_or_escape(const std::filesystem::path &path)
    -> std::string;

}  // namespace logicsim

#endif
