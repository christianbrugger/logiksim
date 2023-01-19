#ifndef LOGIKSIM_COLLISION_H
#define LOGIKSIM_COLLISION_H

#include "geometry.h"

namespace logicsim {

auto point_in_line(point2d_t point, line2d_t line) -> bool;

auto line_points_colliding(line2d_t line0, line2d_t line1) -> bool;

}  // namespace logicsim

#endif