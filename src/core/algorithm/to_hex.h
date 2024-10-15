#ifndef LOGICSIM_ALGORITHM_TO_HEX_H
#define LOGICSIM_ALGORITHM_TO_HEX_H

#include <string>
#include <string_view>

namespace logicsim {

/**
 * @brief: Convert single characters to hex string
 */
[[nodiscard]] auto to_hex(char c) -> std::string;
[[nodiscard]] auto to_hex(wchar_t c) -> std::string;

/**
 * @brief: Convert text to hex string
 */
[[nodiscard]] auto to_hex(std::string_view text) -> std::string;
[[nodiscard]] auto to_hex(std::wstring_view text) -> std::string;

/**
 * @brief: Copy ASCII characters and escapes non ASCII characters as HEX.
 *
 * Note '\' characters are escaped as '\\'
 */
[[nodiscard]] auto to_ascii_or_hex(std::string_view text) -> std::string;
[[nodiscard]] auto to_ascii_or_hex(std::wstring_view text) -> std::string;

}  // namespace logicsim

#endif
