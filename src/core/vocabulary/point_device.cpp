#include "vocabulary/point_device.h"

namespace logicsim {

auto point_device_t::format() const -> std::string {
    return fmt::format("[{}, {}]", x, y);
}

}  // namespace logicsim
