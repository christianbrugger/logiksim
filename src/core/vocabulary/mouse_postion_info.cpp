#include "core/vocabulary/mouse_postion_info.h"

#include "core/format/container.h"
#include "core/vocabulary/point_device_fine.h"
#include "core/vocabulary/point_fine.h"

#include <blend2d/blend2d.h>
#include <fmt/core.h>

namespace logicsim {

auto MousePositionInfo::format() const -> std::string {
    return fmt::format("MousePositionInfo(position = {}, labels = {})", position, labels);
}

auto mouse_position_label(std::string_view label, std::string_view type,
                          point_device_fine_t position) -> std::string {
    return fmt::format("{:.2f}, {:.2f}  {} - {}", position.x, position.y, type, label);
}

auto mouse_position_label(std::string_view label, std::string_view type, BLPoint position)
    -> std::string {
    return fmt::format("{:.2f}, {:.2f}  {} - {}", position.x, position.y, type, label);
}

auto mouse_position_label(std::string_view label, std::string_view type,
                          point_fine_t position) -> std::string {
    return fmt::format("{:.3f}, {:.3f}  {} - {}", double {position.x},
                       double {position.y}, type, label);
}

}  // namespace logicsim
