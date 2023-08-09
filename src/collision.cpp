
#include "collision.h"

#include "geometry.h"

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
    return is_colliding(line0.p0, line1) || is_colliding(line0.p1, line1) ||
           is_colliding(line1.p0, line0) || is_colliding(line1.p1, line0);
}

auto is_colliding(point_t point, rect_t rect) noexcept -> bool {
    return rect.p0.x <= point.x && point.x <= rect.p1.x  //
           && rect.p0.y <= point.y && point.y <= rect.p1.y;
}

auto is_colliding(point_fine_t point, rect_fine_t rect) noexcept -> bool {
    return rect.p0.x <= point.x && point.x <= rect.p1.x  //
           && rect.p0.y <= point.y && point.y <= rect.p1.y;
}

auto is_colliding(ordered_line_t line, rect_t rect) noexcept -> bool {
    // check for no overlap in the x-axis
    if (line.p1.x < rect.p0.x || line.p0.x > rect.p1.x) {
        return false;
    }

    // check for no overlap in the y-axis
    if (line.p1.y < rect.p0.y || line.p0.y > rect.p1.y) {
        return false;
    }

    return true;
}

auto is_colliding(ordered_line_t line, rect_fine_t rect) noexcept -> bool {
    // check for no overlap in the x-axis
    if (line.p1.x < rect.p0.x || line.p0.x > rect.p1.x) {
        return false;
    }

    // check for no overlap in the y-axis
    if (line.p1.y < rect.p0.y || line.p0.y > rect.p1.y) {
        return false;
    }

    return true;
}

auto is_colliding(rect_fine_t a, rect_fine_t b) noexcept -> bool {
    // check for no overlap in the x-axis
    if (a.p1.x < b.p0.x || a.p0.x > b.p1.x) {
        return false;
    }

    // check for no overlap in the y-axis
    if (a.p1.y < b.p0.y || a.p0.y > b.p1.y) {
        return false;
    }

    return true;
}

auto is_colliding(rect_t a, rect_t b) noexcept -> bool {
    // check for no overlap in the x-axis
    if (a.p1.x < b.p0.x || a.p0.x > b.p1.x) {
        return false;
    }

    // check for no overlap in the y-axis
    if (a.p1.y < b.p0.y || a.p0.y > b.p1.y) {
        return false;
    }

    return true;
}

}  // namespace logicsim
