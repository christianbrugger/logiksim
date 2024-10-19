#ifndef LOGICSIM_ALGORITHM_TEXT_ESCAPE_H
#define LOGICSIM_ALGORITHM_TEXT_ESCAPE_H

#include <string>
#include <string_view>

namespace logicsim {

/**
 * @brief: Convert single characters to hex string
 */
[[nodiscard]] auto escape_as_hex(char c) -> std::string;
[[nodiscard]] auto escape_as_hex(wchar_t c) -> std::string;
[[nodiscard]] auto escape_as_hex(char8_t c) -> std::string;
[[nodiscard]] auto escape_as_hex(char16_t c) -> std::string;
[[nodiscard]] auto escape_as_hex(char32_t c) -> std::string;

/**
 * @brief: Convert text to hex string
 */
[[nodiscard]] auto escape_as_hex(std::string_view text) -> std::string;
[[nodiscard]] auto escape_as_hex(std::wstring_view text) -> std::string;
[[nodiscard]] auto escape_as_hex(std::u8string_view text) -> std::string;
[[nodiscard]] auto escape_as_hex(std::u16string_view text) -> std::string;
[[nodiscard]] auto escape_as_hex(std::u32string_view text) -> std::string;

/**
 * @brief: Escape non printable ASCII characters as hex.
 *
 * Note '\' characters are escaped as '\\'
 */
[[nodiscard]] auto escape_as_ascii_or_hex(std::string_view text) -> std::string;
[[nodiscard]] auto escape_as_ascii_or_hex(std::wstring_view text) -> std::string;
[[nodiscard]] auto escape_as_ascii_or_hex(std::u8string_view text) -> std::string;
[[nodiscard]] auto escape_as_ascii_or_hex(std::u16string_view text) -> std::string;
[[nodiscard]] auto escape_as_ascii_or_hex(std::u32string_view text) -> std::string;

}  // namespace logicsim

#endif
