#include "vocabulary/rect_fine.h"

#include <fmt/core.h>

namespace logicsim {

auto rect_fine_t::format() const -> std::string {
    return fmt::format("RectFine({}, {})", p0, p1);
}

}  // namespace logicsim
