#include "algorithm/u8_conversion.h"

namespace logicsim {

auto to_u8string(std::string_view filename) -> std::u8string {
    auto result = std::u8string {};

    result.resize(filename.size());
    std::ranges::copy(filename, result.begin());

    return result;
}

}  // namespace logicsim
