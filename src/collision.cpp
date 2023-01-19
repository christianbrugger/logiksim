
#include "collision.h"

namespace logicsim {

auto point_in_line(point2d_t point, line2d_t line) -> bool {
    auto p0 = line.p0;
    auto p1 = line.p1;

    if (p0.x == p1.x) {
        // horizontal
        if (point.x != p0.x) {
            return false;
        }
        if (p0.y <= p1.y) {
            return p0.y <= point.y && point.y <= p1.y;
        }
        return p1.y <= point.y && point.y <= p0.y;
    }
    // vertical
    assert(p0.y == p1.y);
    if (point.y != p0.y) {
        return false;
    }
    if (p0.x <= p1.x) {
        return p0.x <= point.x && point.x <= p1.x;
    }
    return p1.x <= point.x && point.x <= p0.x;
}

auto line_points_colliding(line2d_t line0, line2d_t line1) -> bool {
    return point_in_line(line0.p0, line1) || point_in_line(line0.p1, line1)
           || point_in_line(line1.p0, line0) || point_in_line(line1.p1, line0);
}

}  // namespace logicsim
