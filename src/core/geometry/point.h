#ifndef LOGICSIM_GEOMETRY_POINT_H
#define LOGICSIM_GEOMETRY_POINT_H

#include "core/vocabulary/point.h"

#include <span>
#include <vector>

namespace logicsim {

[[nodiscard]] auto is_representable(point_t point, int dx, int dy) -> bool;

[[nodiscard]] auto add_unchecked(point_t point, int dx, int dy) -> point_t;

[[nodiscard]] auto move_or_delete_points(std::span<const point_t> points, int delta_x,
                                         int delta_y) -> std::vector<point_t>;

}  // namespace logicsim

#endif
