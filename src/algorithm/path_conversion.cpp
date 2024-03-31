#include "algorithm/path_conversion.h"

namespace logicsim {

namespace {

auto to_u8string(std::string_view filename) -> std::u8string {
    auto result = std::u8string {};

    result.resize(filename.size());
    std::ranges::copy(filename, result.begin());

    return result;
}

}  // namespace

auto to_path(std::string_view filename) -> std::filesystem::path {
    return std::filesystem::path {to_u8string(filename)};
}

}  // namespace logicsim
