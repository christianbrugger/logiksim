#ifndef LOGIKSIM_GEOMETRY_H
#define LOGIKSIM_GEOMETRY_H

#include "algorithm.h"
#include "circuit_vocabulary.h"
#include "exceptions.h"

#include <fmt/core.h>

#include <cassert>
#include <cstdint>
#include <tuple>

namespace logicsim {

constexpr auto is_horizontal(line2d_t line) noexcept -> bool {
    return line.p0.y == line.p1.y;
}

constexpr auto is_vertical(line2d_t line) noexcept -> bool {
    return line.p0.x == line.p1.x;
}

constexpr auto is_orthogonal(line2d_t line) noexcept -> bool {
    return is_horizontal(line) || is_vertical(line);
}

// order points within lines
constexpr auto order_points(line2d_t line) noexcept {
    auto [p0, p1] = sorted(line.p0, line.p1);
    return line2d_t {p0, p1};
}

// order lines and points within lines
constexpr auto order_points(const line2d_t line0, const line2d_t line1) noexcept
    -> std::tuple<line2d_t, line2d_t> {
    auto a = order_points(line0);
    auto b = order_points(line1);

    if (a.p0 <= b.p0) {
        return std::tie(a, b);
    }
    return std::tie(b, a);
}

constexpr auto is_vertical(orthogonal_line_t line) noexcept -> bool {
    return line.p0.x == line.p1.x;
}

constexpr auto is_horizontal(orthogonal_line_t line) noexcept -> bool {
    return !is_vertical(line);
}

// fast distance for horitonal or vertical lines
inline auto distance_1d(point2d_t p0, point2d_t p1) -> int {
    auto dx = p1.x - p0.x;
    auto dy = p1.y - p0.y;
    assert(dx == 0 || dy == 0);
    return std::abs((dx == 0) ? dy : dx);
}

// fast distance for horitonal or vertical lines
inline auto distance_1d(line2d_t line) -> int {
    return distance_1d(line.p0, line.p1);
}

}  // namespace logicsim

#endif