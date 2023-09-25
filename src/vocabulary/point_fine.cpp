#include "vocabulary/point_fine.h"

#include <fmt/core.h>

namespace logicsim {

auto point_fine_t::format() const -> std::string {
    return fmt::format("[{:.3f}, {:.3f}]", x, y);
}

}  // namespace logicsim
