#include "core/vocabulary/rect.h"

#include <fmt/format.h>

namespace logicsim {

auto rect_t::format() const -> std::string {
    return fmt::format("Rect({}, {})", p0, p1);
}

}  // namespace logicsim
