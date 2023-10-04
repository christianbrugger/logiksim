#include "geometry/point.h"

#include "geometry/grid.h"
#include "vocabulary/point.h"

namespace logicsim {

auto add_unchecked(point_t point, int dx, int dy) -> point_t {
    return point_t {
        add_unchecked(point.x, dx),
        add_unchecked(point.y, dy),
    };
}

}  // namespace logicsim
