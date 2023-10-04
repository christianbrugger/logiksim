#ifndef LOGICSIM_FORMAT_ESCAPE_ASCII_H
#define LOGICSIM_FORMAT_ESCAPE_ASCII_H

#include <fmt/core.h>

#include <string>
#include <type_traits>

namespace logicsim {

template <class CharT, class Traits, class Allocator>
auto escape_non_ascii(const std::basic_string<CharT, Traits, Allocator> &input)
    -> std::string {
    using unsigned_integer_type = std::make_unsigned_t<CharT>;
    constexpr auto hex_zeros = sizeof(unsigned_integer_type) * 2;

    auto result = std::string {};
    for (const auto &character : input) {
        const auto index = static_cast<unsigned_integer_type>(character);
        if (index <= 31 || index >= 127) {
            fmt::format_to(std::back_inserter(result), "\\x{:0{}x}", index, hex_zeros);
        } else {
            result.push_back(character);
        }
    }
    return result;
}

}  // namespace logicsim

#endif
