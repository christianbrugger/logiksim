#ifndef LOGIKSIM_COLLISION_H
#define LOGIKSIM_COLLISION_H

#include "geometry.h"

namespace logicsim {

// is the point inside the line
auto is_inside(point_t point, ordered_line_t line) noexcept -> bool;

// is the point colliding with the line
auto is_colliding(point_t point, ordered_line_t line) noexcept -> bool;

// are the line endpoints colliding with the other line
auto line_points_colliding(ordered_line_t line0, ordered_line_t line1) noexcept -> bool;

}  // namespace logicsim

#endif