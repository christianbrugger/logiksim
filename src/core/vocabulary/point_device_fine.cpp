#include "core/vocabulary/point_device_fine.h"

namespace logicsim {

auto point_device_fine_t::format() const -> std::string {
    return fmt::format("[{}, {}]", x, y);
}

}  // namespace logicsim
