#ifndef LOGICSIM_VOCABULARY_MOUSE_POSTION_INFO_H
#define LOGICSIM_VOCABULARY_MOUSE_POSTION_INFO_H

#include "core/format/struct.h"
#include "core/vocabulary/point_device_fine.h"

#include <concepts>
#include <vector>

struct BLPoint;

namespace logicsim {

struct point_device_fine_t;
struct point_fine_t;

struct MousePositionInfo {
    point_device_fine_t position {};
    std::vector<std::string> labels {};

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] auto operator==(const MousePositionInfo&) const -> bool = default;
};

static_assert(std::regular<MousePositionInfo>);

[[nodiscard]] auto mouse_position_label(std::string_view label, std::string_view type,
                                        point_device_fine_t position) -> std::string;
[[nodiscard]] auto mouse_position_label(std::string_view label, std::string_view type,
                                        BLPoint position) -> std::string;
[[nodiscard]] auto mouse_position_label(std::string_view label, std::string_view type,
                                        point_fine_t position) -> std::string;

}  // namespace logicsim

#endif
