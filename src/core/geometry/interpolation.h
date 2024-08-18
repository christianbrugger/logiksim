#ifndef LOGICSIM_GEOMETRY_INTERPOLATION_H
#define LOGICSIM_GEOMETRY_INTERPOLATION_H

namespace logicsim {

struct grid_t;
struct grid_fine_t;
struct point_t;
struct point_fine_t;
struct time_t;

[[nodiscard]] auto interpolate_1d(grid_t v0, grid_t v1, double ratio) -> grid_fine_t;
[[nodiscard]] auto interpolate_line_1d(point_t p0, point_t p1, time_t t0, time_t t1,
                                       time_t t_select) -> point_fine_t;

}  // namespace logicsim

#endif
