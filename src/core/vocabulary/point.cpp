#include "vocabulary/point.h"

namespace logicsim {

auto point_t::format() const -> std::string {
    return fmt::format("[{}, {}]", x, y);
}

}  // namespace logicsim
