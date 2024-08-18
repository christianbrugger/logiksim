#include "random/point.h"

#include "random/grid.h"
#include "vocabulary/grid.h"
#include "vocabulary/point.h"
#include "vocabulary/rect.h"

namespace logicsim {

auto get_random_point(Rng& rng) -> point_t {
    return get_random_point(rng, grid_t::min(), grid_t::max());
}

auto get_random_point(Rng& rng, grid_t min, grid_t max) -> point_t {
    return point_t {
        get_random_grid(rng, min, max),
        get_random_grid(rng, min, max),
    };
}

auto get_random_point(Rng& rng, rect_t rect) -> point_t {
    return point_t {
        get_random_grid(rng, rect.p0.x, rect.p1.x),
        get_random_grid(rng, rect.p0.y, rect.p1.y),
    };
}

}  // namespace logicsim
