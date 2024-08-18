#include "geometry/rect.h"

#include "geometry/grid.h"
#include "vocabulary/grid.h"
#include "vocabulary/grid_fine.h"
#include "vocabulary/line.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/point.h"
#include "vocabulary/point_fine.h"
#include "vocabulary/rect.h"
#include "vocabulary/rect_fine.h"

namespace logicsim {

auto enclosing_rect(rect_fine_t rect) -> rect_t {
    return rect_t {
        point_t {
            to_floored(rect.p0.x),
            to_floored(rect.p0.y),
        },
        point_t {
            to_ceiled(rect.p1.x),
            to_ceiled(rect.p1.y),
        },
    };
}

auto enclosing_rect(rect_t a, rect_t b) -> rect_t {
    return rect_t {
        point_t {std::min(a.p0.x, b.p0.x), std::min(a.p0.y, b.p0.y)},
        point_t {std::max(a.p1.x, b.p1.x), std::max(a.p1.y, b.p1.y)},
    };
}

auto enclosing_rect(rect_t rect, ordered_line_t line) -> rect_t {
    return rect_t {
        point_t {std::min(rect.p0.x, line.p0.x), std::min(rect.p0.y, line.p0.y)},
        point_t {std::max(rect.p1.x, line.p1.x), std::max(rect.p1.y, line.p1.y)},
    };
}

auto to_rect(point_fine_t center, grid_fine_t size) -> rect_fine_t {
    const auto half = size / 2.0;

    return rect_fine_t {
        point_fine_t {center.x - half, center.y - half},
        point_fine_t {center.x + half, center.y + half},
    };
}

auto get_center(rect_fine_t rect) -> point_fine_t {
    return point_fine_t {
        (rect.p0.x + rect.p1.x) / 2.0,
        (rect.p0.y + rect.p1.y) / 2.0,
    };
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
