#include "core/vocabulary/grid.h"

#include <fmt/format.h>

namespace logicsim {

auto grid_t::format() const -> std::string {
    return fmt::format("{}", value);
}

}  // namespace logicsim
