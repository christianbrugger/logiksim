#ifndef LOGICSIM_ALGORITHM_U8_CONVERSION_H
#define LOGICSIM_ALGORITHM_U8_CONVERSION_H

#include <string>
#include <string_view>

namespace logicsim {

/**
 * @brief: Casts text containing UTF-8 data to a u8string.
 */
[[nodiscard]] auto to_u8string(std::string_view text) -> std::u8string;

/**
 * @brief: Casts u8-text to a string containing UTF-8 data.
 */
[[nodiscard]] auto to_string(std::u8string_view text) -> std::string;

}  // namespace logicsim

#endif
