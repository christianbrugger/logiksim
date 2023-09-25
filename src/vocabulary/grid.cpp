#include "vocabulary/grid.h"

#include <fmt/core.h>

namespace logicsim {

auto grid_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
