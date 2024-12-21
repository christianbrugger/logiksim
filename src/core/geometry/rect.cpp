#include "core/geometry/rect.h"

#include "core/geometry/grid.h"
#include "core/vocabulary/grid.h"
#include "core/vocabulary/grid_fine.h"
#include "core/vocabulary/line.h"
#include "core/vocabulary/ordered_line.h"
#include "core/vocabulary/point.h"
#include "core/vocabulary/point_fine.h"
#include "core/vocabulary/rect.h"
#include "core/vocabulary/rect_fine.h"

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

auto enclosing_rect(const std::optional<rect_t>& a,
                    const std::optional<rect_t>& b) -> std::optional<rect_t> {
    if (!a.has_value() && !b.has_value()) {
        return std::nullopt;
    }
    if (!a.has_value()) {
        return b;
    }
    if (!b.has_value()) {
        return a;
    }

    return enclosing_rect(a.value(), b.value());
};

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

auto enlarge_rect(rect_t rect, grid_fine_t margin) -> rect_fine_t {
    return rect_fine_t {
        point_fine_t {rect.p0.x - margin, rect.p0.y - margin},
        point_fine_t {rect.p1.x + margin, rect.p1.y + margin},
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

auto a_inside_b(rect_t a, rect_t b) noexcept -> bool {
    return (a.p0.x >= b.p0.x && a.p0.y >= b.p0.y) &&
           (a.p1.x <= b.p1.x && a.p1.y <= b.p1.y);
}

}  // namespace logicsim
