#include "vocabulary/mouse_postion_info.h"

#include "format/container.h"

#include <fmt/core.h>

namespace logicsim {

auto position_source_point_t::format() const -> std::string {
    return fmt::format("({}, {})", x, y);
}

auto position_sources_entry_t::format() const -> std::string {
    return fmt::format("{}: {}", title, position);
}

auto MousePositionInfo::format() const -> std::string {
    return fmt::format("MousePositionInfo(final_postition = {}, sources = {})",
                       final_position, sources);
}

}  // namespace logicsim
