#ifndef LOGIKSIM_COLLISION_H
#define LOGIKSIM_COLLISION_H

#include "geometry.h"

namespace logicsim {

// is the point inside the line, excluding endpoints
[[nodiscard]] auto is_inside(point_t point, ordered_line_t line) noexcept -> bool;

// is the point colliding with the line
[[nodiscard]] auto is_colliding(point_t point, ordered_line_t line) noexcept -> bool;

// are the line endpoints colliding with the other line
[[nodiscard]] auto line_points_colliding(ordered_line_t line0,
                                         ordered_line_t line1) noexcept -> bool;

[[nodiscard]] auto is_colliding(rect_fine_t a, rect_fine_t b) noexcept -> bool;
[[nodiscard]] auto is_colliding(rect_t a, rect_t b) noexcept -> bool;

}  // namespace logicsim

#endif