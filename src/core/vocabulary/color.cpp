#include "core/vocabulary/color.h"

#include <blend2d.h>
#include <fmt/core.h>

namespace logicsim {

auto color_t::format() const -> std::string {
    return fmt::format("{:X}", value);
}

color_t::operator BLRgba32() const noexcept {
    return BLRgba32 {value};
}

}  // namespace logicsim
