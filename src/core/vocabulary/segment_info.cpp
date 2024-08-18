#include "vocabulary/segment_info.h"

#include <fmt/core.h>

namespace logicsim {

auto segment_info_t::format() const -> std::string {
    return fmt::format("({} {} - {} {})", p0_type, line.p0, line.p1, p1_type);
}

}  // namespace logicsim
