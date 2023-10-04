#include "geometry.h"

#include "algorithm/round.h"
#include "exception.h"
#include "geometry.h"
#include "vocabulary.h"

#include <numbers>

namespace logicsim {

auto interpolate_1d(grid_t v0, grid_t v1, double ratio) -> grid_fine_t {
    return grid_fine_t {v0.value + (v1.value - v0.value) * ratio};
}

auto interpolate_line_1d(point_t p0, point_t p1, time_t t0, time_t t1, time_t t_select)
    -> point_fine_t {
    assert(t0 < t1);

    if (t_select <= t0) {
        return point_fine_t {p0};
    }
    if (t_select >= t1) {
        return point_fine_t {p1};
    }

    const double alpha = static_cast<double>((t_select.value - t0.value).count()) /
                         static_cast<double>((t1.value - t0.value).count());

    if (is_horizontal(line_t {p0, p1})) {
        return point_fine_t {interpolate_1d(p0.x, p1.x, alpha), grid_fine_t(p0.y)};
    }
    return point_fine_t {grid_fine_t {p0.x}, interpolate_1d(p0.y, p1.y, alpha)};
}

}  // namespace logicsim