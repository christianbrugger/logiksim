#include "algorithm/path_conversion.h"

#include "algorithm/text_escape.h"
#include "algorithm/u8_conversion.h"

namespace logicsim {

namespace {

[[nodiscard]] auto path_to_utf8_u8(const std::filesystem::path &path)
    -> std::optional<std::u8string> {
    try {
        return path.u8string();
    } catch (const std::exception &) {
    }
    return std::nullopt;
}

}  // namespace

auto path_to_utf8(const std::filesystem::path &path) -> std::optional<std::string> {
    if (const auto result = path_to_utf8_u8(path); result) {
        return to_string(*result);
    }
    return std::nullopt;
}

auto path_to_utf8_or_escape(const std::filesystem::path &path) -> std::string {
    if (const auto result = path_to_utf8(path); result) {
        return *result;
    }
    return to_ascii_or_hex(path.native());
}

}  // namespace logicsim
