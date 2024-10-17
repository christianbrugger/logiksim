#include "core/vocabulary/grid_fine.h"

#include <fmt/core.h>

namespace logicsim {

auto grid_fine_t::format() const -> std::string {
    return fmt::format("{:.3f}", value);
}

}  // namespace logicsim
