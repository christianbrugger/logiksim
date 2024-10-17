#include "core/vocabulary/device_pixel_ratio.h"

#include <fmt/core.h>

namespace logicsim {

auto device_pixel_ratio_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
