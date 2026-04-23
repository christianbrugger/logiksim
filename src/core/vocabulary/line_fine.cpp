#include "core/vocabulary/line_fine.h"

#include <fmt/format.h>

namespace logicsim {

auto line_fine_t::format() const -> std::string {
    return fmt::format("LineFine({}, {})", p0, p1);
}

}  // namespace logicsim
