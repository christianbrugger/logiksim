#include "core/executable_path.h"

#include "core/algorithm/u8_conversion.h"

#include <gsl/gsl>
#include <whereami.h>

#include <stdexcept>

namespace logicsim {

auto get_executable_directory() -> std::filesystem::path {
    const auto size = wai_getExecutablePath(nullptr, 0, nullptr);

    auto buffer = std::string(gsl::narrow<std::size_t>(size), '\0');

    auto dirname_length = int {0};
    Expects(size == wai_getExecutablePath(buffer.data(), size, &dirname_length));
    Expects(dirname_length <= size);

    const auto folder_view =
        std::string_view {buffer}.substr(0, gsl::narrow<std::size_t>(dirname_length));

    return std::filesystem::path {to_u8string(folder_view)};
}

}  // namespace logicsim
