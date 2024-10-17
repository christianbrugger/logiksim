#include "core/geometry/interpolation.h"

#include "core/geometry/orientation.h"
#include "core/vocabulary/grid.h"
#include "core/vocabulary/grid_fine.h"
#include "core/vocabulary/line.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/point_fine.h"
#include "core/vocabulary/time.h"

#include <cassert>

namespace logicsim {

auto interpolate_1d(grid_t v0, grid_t v1, double ratio) -> grid_fine_t {
    static_assert(sizeof(int) > sizeof(grid_t::value_type));
    return grid_fine_t {int {v0} + (int {v1} - int {v0}) * ratio};
}

auto interpolate_line_1d(point_t p0, point_t p1, time_t t0, time_t t1,
                         time_t t_select) -> point_fine_t {
    assert(t0 < t1);

    if (t_select <= t0) {
        return point_fine_t {p0};
    }
    if (t_select >= t1) {
        return point_fine_t {p1};
    }

    // TODO check cast ???
    const double alpha = static_cast<double>((t_select - t0).count_ns()) /
                         static_cast<double>((t1 - t0).count_ns());

    if (is_horizontal(line_t {p0, p1})) {
        return point_fine_t {interpolate_1d(p0.x, p1.x, alpha), grid_fine_t(p0.y)};
    }
    return point_fine_t {grid_fine_t {p0.x}, interpolate_1d(p0.y, p1.y, alpha)};
}

}  // namespace logicsim
