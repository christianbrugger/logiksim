#include "core/vocabulary/line.h"

#include <fmt/format.h>

namespace logicsim {

auto line_t::format() const -> std::string {
    return fmt::format("Line({}, {})", p0, p1);
}

}  // namespace logicsim
