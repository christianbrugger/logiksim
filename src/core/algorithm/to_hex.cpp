#include "algorithm/to_hex.h"

#include <fmt/core.h>
#include <gsl/gsl>

#include <cstdint>

namespace logicsim {

namespace {

template <typename Target, typename Source>
constexpr auto same_size_cast(Source c) -> Target {
    static_assert(sizeof(Target) == sizeof(Source));
    return static_cast<Target>(c);
}

template <typename CharType>
auto generic_char_to_hex(CharType c) -> std::string {
    if constexpr (sizeof(CharType) == 1) {
        return fmt::format("\\x{:02x}", same_size_cast<uint8_t>(c));
    } else if constexpr (sizeof(CharType) == 2) {
        return fmt::format("\\u{:04x}", same_size_cast<uint16_t>(c));
    } else if constexpr (sizeof(CharType) == 4) {
        return fmt::format("\\U{:08x}", same_size_cast<uint32_t>(c));
    } else {
        static_assert(false);
    }
}

template <typename CharType>
auto generic_is_ascii(CharType c) -> bool {
    return 0 <= c && c <= 127;
}

template <typename CharType>
auto generic_char_to_ascii_hex(CharType c) -> std::string {
    if (c == '\\') {
        return std::string {"\\\\"};
    }
    if (generic_is_ascii(c)) {
        return std::string {gsl::narrow_cast<char>(c)};
    }
    return generic_char_to_hex(c);
}

template <typename CharType>
auto generic_string_view_to_hex(std::basic_string_view<CharType> text) -> std::string {
    auto result = std::string {};

    for (const auto& c : text) {
        result += generic_char_to_hex(c);
    }

    return result;
}

template <typename CharType>
auto generic_string_view_to_ascii_hex(std::basic_string_view<CharType> text)
    -> std::string {
    auto result = std::string {};

    for (const auto& c : text) {
        result += generic_char_to_ascii_hex(c);
    }

    return result;
}

}  // namespace

auto to_hex(char c) -> std::string {
    return generic_char_to_hex(c);
}

auto to_hex(wchar_t c) -> std::string {
    return generic_char_to_hex(c);
}

auto to_hex(std::string_view text) -> std::string {
    return generic_string_view_to_hex(text);
}

auto to_hex(std::wstring_view text) -> std::string {
    return generic_string_view_to_hex(text);
}

auto to_ascii_or_hex(std::string_view text) -> std::string {
    return generic_string_view_to_ascii_hex(text);
}

auto to_ascii_or_hex(std::wstring_view text) -> std::string {
    return generic_string_view_to_ascii_hex(text);
}

}  // namespace logicsim
