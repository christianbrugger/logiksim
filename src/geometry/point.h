#ifndef LOGICSIM_GEOMETRY_POINT_H
#define LOGICSIM_GEOMETRY_POINT_H

namespace logicsim {

struct point_t;

[[nodiscard]] auto add_unchecked(point_t point, int dx, int dy) -> point_t;

}  // namespace logicsim

#endif