#include "vocabulary/size_device.h"

namespace logicsim {

auto size_device_t::format() const -> std::string {
    return fmt::format("[{}, {}]", x, y);
}

}  // namespace logicsim
