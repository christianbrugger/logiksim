#include "location.h"

#include "algorithm/path_conversion.h"

#include <gsl/gsl>
#include <whereami.h>

#include <stdexcept>

namespace logicsim {

auto get_executable_path() -> std::filesystem::path {
    const auto size = wai_getExecutablePath(nullptr, 0, nullptr);

    auto buffer = std::string(gsl::narrow<std::size_t>(size), '\0');

    auto dirname_length = int {0};
    Expects(size == wai_getExecutablePath(buffer.data(), size, &dirname_length));
    Expects(dirname_length <= size);

    return to_path(
        std::string_view {buffer}.substr(0, gsl::narrow<std::size_t>(dirname_length)));
}

}  // namespace logicsim
