#include "algorithm/u8_conversion.h"

namespace logicsim {

auto to_u8string(std::string_view text) -> std::u8string {
    auto result = std::u8string {};

    result.resize(text.size());
    std::ranges::copy(text, result.begin());

    return result;
}

auto to_string(std::u8string_view text) -> std::string {
    auto result = std::string {};

    result.resize(text.size());
    std::ranges::copy(text, result.begin());

    return result;
}

}  // namespace logicsim
