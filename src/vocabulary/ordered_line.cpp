#include "vocabulary/ordered_line.h"

#include <fmt/core.h>

namespace logicsim {

auto ordered_line_t::format() const -> std::string {
    return fmt::format("OrderedLine({}, {})", p0, p1);
}

}  // namespace logicsim
