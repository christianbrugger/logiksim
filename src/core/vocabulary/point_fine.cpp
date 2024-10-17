#include "core/vocabulary/point_fine.h"

#include <fmt/core.h>

namespace logicsim {

auto point_fine_t::format() const -> std::string {
    return fmt::format("[{}, {}]", x, y);
}

}  // namespace logicsim
