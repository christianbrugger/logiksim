#include "algorithm/to_path.h"

#include "algorithm/u8_conversion.h"

namespace logicsim {

auto to_path(std::string_view text) -> std::filesystem::path {
    return std::filesystem::path {to_u8string(text)};
}

}  // namespace logicsim
