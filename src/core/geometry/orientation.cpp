#include "geometry/orientation.h"

#include "vocabulary/line.h"
#include "vocabulary/ordered_line.h"
#include "vocabulary/point.h"

#include <exception>
#include <numbers>
#include <stdexcept>

namespace logicsim {

auto is_horizontal(orientation_t orientation) noexcept -> bool {
    using enum orientation_t;
    return orientation == left || orientation == right;
}

auto is_vertical(orientation_t orientation) noexcept -> bool {
    using enum orientation_t;
    return orientation == up || orientation == down;
}

auto is_horizontal(point_t p0, point_t p1) noexcept -> bool {
    return p0.y == p1.y;
}

auto is_vertical(point_t p0, point_t p1) noexcept -> bool {
    return p0.x == p1.x;
}

auto is_horizontal(line_t line) noexcept -> bool {
    return line.p0.y == line.p1.y;
}

auto is_vertical(line_t line) noexcept -> bool {
    return line.p0.x == line.p1.x;
}

auto is_horizontal(ordered_line_t line) noexcept -> bool {
    return is_horizontal(line_t {line});
}

auto is_vertical(ordered_line_t line) noexcept -> bool {
    return is_vertical(line_t {line});
}

auto to_orientation(point_t p0, point_t p1) -> orientation_t {
    using enum orientation_t;

    if (p1.x > p0.x) {
        return orientation_t::right;
    }
    if (p1.x < p0.x) {
        return orientation_t::left;
    }
    if (p1.y < p0.y) {
        return orientation_t::up;
    }
    if (p1.y > p0.y) {
        return orientation_t::down;
    }

    throw std::runtime_error("p0 and p1 cannot be qual");
}

auto to_orientation_p0(ordered_line_t line) -> orientation_t {
    return to_orientation(line.p1, line.p0);
}

auto to_orientation_p1(ordered_line_t line) -> orientation_t {
    return to_orientation(line.p0, line.p1);
}

auto to_orientation_p0(line_t line) -> orientation_t {
    return to_orientation(line.p1, line.p0);
}

auto to_orientation_p1(line_t line) -> orientation_t {
    return to_orientation(line.p0, line.p1);
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
            throw std::runtime_error("undirected orientation has no angle");
    };
    std::terminate();
}

auto orientations_compatible(orientation_t a, orientation_t b) -> bool {
    using enum orientation_t;
    return (a == left && b == right) || (a == right && b == left) ||
           (a == up && b == down) || (a == down && b == up) || (a == undirected) ||
           (b == undirected);
}

}  // namespace logicsim
