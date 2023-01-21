#ifndef LOGIKSIM_COLLISION_H
#define LOGIKSIM_COLLISION_H

#include "geometry.h"

namespace logicsim {

// is the point inside the line
auto is_inside(point2d_t point, line2d_t line) noexcept -> bool;

// is the point colliding with the line
auto is_colliding(point2d_t point, line2d_t line) noexcept -> bool;

// are the line endpoints colliding with the other line
auto line_points_colliding(line2d_t line0, line2d_t line1) noexcept -> bool;

}  // namespace logicsim

#endif