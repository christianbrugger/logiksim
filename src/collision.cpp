
#include "collision.h"

namespace logicsim {

auto is_inside(point_t point, ordered_line_t line) noexcept -> bool {
    bool horizontal = is_horizontal(line);
    if ((horizontal && point.y != line.p0.y) || (!horizontal && point.x != line.p0.x)) {
        return false;
    }

    if (horizontal) {
        return line.p0.x < point.x && point.x < line.p1.x;
    }
    return line.p0.y < point.y && point.y < line.p1.y;
}

auto is_colliding(point_t point, ordered_line_t line) noexcept -> bool {
    if (is_horizontal(line)) {
        return point.y == line.p0.y && line.p0.x <= point.x && point.x <= line.p1.x;
    }
    return point.x == line.p0.x && line.p0.y <= point.y && point.y <= line.p1.y;
}

auto line_points_colliding(ordered_line_t line0, ordered_line_t line1) noexcept -> bool {
    return is_colliding(line0.p0, line1) || is_colliding(line0.p1, line1)
           || is_colliding(line1.p0, line0) || is_colliding(line1.p1, line0);
}

}  // namespace logicsim
