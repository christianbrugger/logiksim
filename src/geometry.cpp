#include "geometry.h"

#include "algorithm.h"
#include "exceptions.h"
#include "vocabulary.h"

#include <numbers>

namespace logicsim {

auto is_horizontal(line_t line) noexcept -> bool {
    return line.p0.y == line.p1.y;
}

auto is_vertical(line_t line) noexcept -> bool {
    return line.p0.x == line.p1.x;
}

// order points within lines
auto order_points(line_t line) noexcept -> line_t {
    auto [p0, p1] = sorted(line.p0, line.p1);
    return line_t {p0, p1};
}

// order lines and points within lines
auto order_points(const line_t line0, const line_t line1) noexcept
    -> std::tuple<line_t, line_t> {
    auto a = order_points(line0);
    auto b = order_points(line1);

    if (a.p0 <= b.p0) {
        return std::tie(a, b);
    }
    return std::tie(b, a);
}

// fast distance for horitonal or vertical lines
auto distance(line_t line) -> int {
    auto dx = line.p1.x.value - line.p0.x.value;
    auto dy = line.p1.y.value - line.p0.y.value;

    // ensure enough precision, through promotion
    static_assert(sizeof(dx) > sizeof(line.p0.x.value));
    return std::abs((dx == 0) ? dy : dx);
}

auto to_orientation(point_t p0, point_t p1) -> orientation_t {
    return to_orientation(line_t {p0, p1});
}

auto to_orientation(line_t line) -> orientation_t {
    using enum orientation_t;

    if (line.p1.x > line.p0.x) {
        return orientation_t::right;
    }
    if (line.p1.x < line.p0.x) {
        return orientation_t::left;
    }
    if (line.p1.y < line.p0.y) {
        return orientation_t::up;
    }
    if (line.p1.y > line.p0.y) {
        return orientation_t::down;
    }

    throw_exception("unreachable code");
}

auto to_angle(orientation_t orientation) -> double {
    switch (orientation) {
        using enum orientation_t;
        case left:
            return 0.;
        case right:
            return std::numbers::pi;
        case up:
            return std::numbers::pi / 2;
        case down:
            return std::numbers::pi * 3 / 2;
        case undirected:
            throw_exception("can't draw arrow for undirected orientation");
    };
    throw_exception("unhandeled orientation");
}

}  // namespace logicsim