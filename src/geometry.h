#ifndef LOGIKSIM_GEOMETRY_H
#define LOGIKSIM_GEOMETRY_H

#include "exception.h"
#include "geometry/grid.h"
#include "geometry/line.h"
#include "geometry/offset.h"
#include "geometry/orientation.h"
#include "geometry/part.h"
#include "geometry/part_list.h"
#include "geometry/part_list_copying.h"
#include "geometry/point.h"
#include "geometry/rect.h"
#include "vocabulary.h"

#include <tuple>

namespace logicsim {

[[nodiscard]] auto interpolate_1d(grid_t v0, grid_t v1, double ratio) -> grid_fine_t;
[[nodiscard]] auto interpolate_line_1d(point_t p0, point_t p1, time_t t0, time_t t1,
                                       time_t t_select) -> point_fine_t;

}  // namespace logicsim

#endif