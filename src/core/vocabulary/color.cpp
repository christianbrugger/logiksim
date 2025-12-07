#include "core/vocabulary/color.h"

#include <blend2d/blend2d.h>
#include <fmt/core.h>

namespace logicsim {

auto color_t::format() const -> std::string {
    if (is_rgb(*this)) {
        return fmt::format("0x{:06x}", value & value_type {0xFFFFFF});
    }
    return fmt::format("0x{:08x}", value);
}

color_t::operator BLRgba32() const noexcept {
    return BLRgba32 {value};
}

}  // namespace logicsim
