#include "line.h"

#include "geometry/line.h"
#include "geometry/orientation.h"
#include "geometry/point.h"
#include "vocabulary/grid.h"
#include "vocabulary/line.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/point.h"

#include <cmath>

namespace logicsim {

auto distance(line_t line) -> int {
    const auto dx = int {line.p1.x} - int {line.p0.x};
    const auto dy = int {line.p1.y} - int {line.p0.y};

    // ensure enough precision through promotion
    static_assert(sizeof(dx) > sizeof(grid_t::value_type));

    return std::abs(dx == 0 ? dy : dx);
}

auto distance(ordered_line_t line) -> int {
    return distance(line_t {line});
}

auto order_points(line_t line0, line_t line1) noexcept
    -> std::tuple<ordered_line_t, ordered_line_t> {
    auto a = ordered_line_t {line0};
    auto b = ordered_line_t {line1};

    if (a.p0 <= b.p0) {
        return std::make_tuple(a, b);
    }
    return std::make_tuple(b, a);
}

auto is_endpoint(point_t point, line_t line) -> bool {
    return line.p0 == point || line.p1 == point;
}

auto is_endpoint(point_t point, ordered_line_t line) -> bool {
    return is_endpoint(point, line_t {line});
}

auto is_representable(line_t line, int dx, int dy) -> bool {
    return is_representable(line.p0, dx, dy) && is_representable(line.p1, dx, dy);
}

auto is_representable(ordered_line_t line, int dx, int dy) -> bool {
    return is_representable(line_t {line}, dx, dy);
}

auto add_unchecked(line_t line, int dx, int dy) -> line_t {
    return line_t {
        add_unchecked(line.p0, dx, dy),
        add_unchecked(line.p1, dx, dy),
    };
}

auto add_unchecked(ordered_line_t line, int dx, int dy) -> ordered_line_t {
    return ordered_line_t {
        add_unchecked(line.p0, dx, dy),
        add_unchecked(line.p1, dx, dy),
    };
}

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

auto is_inside(point_t point, line_t line) noexcept -> bool {
    return is_inside(point, ordered_line_t {line});
}

auto is_colliding(point_t point, ordered_line_t line) noexcept -> bool {
    if (is_horizontal(line)) {
        return point.y == line.p0.y && line.p0.x <= point.x && point.x <= line.p1.x;
    }
    return point.x == line.p0.x && line.p0.y <= point.y && point.y <= line.p1.y;
}

auto is_colliding(point_t point, line_t line) noexcept -> bool {
    return is_colliding(point, ordered_line_t {line});
}

auto line_points_colliding(ordered_line_t line0, ordered_line_t line1) noexcept -> bool {
    return is_colliding(line0.p0, line1) || is_colliding(line0.p1, line1) ||
           is_colliding(line1.p0, line0) || is_colliding(line1.p1, line0);
}

auto line_points_colliding(line_t line0, line_t line1) noexcept -> bool {
    return line_points_colliding(ordered_line_t {line0}, ordered_line_t {line1});
}

auto lines_parallel(line_t line0, line_t line1) -> bool {
    return is_horizontal(line0) == is_horizontal(line1);
}

auto lines_orthogonal(line_t line0, line_t line1) -> bool {
    return is_horizontal(line0) != is_horizontal(line1);
}

}  // namespace logicsim
