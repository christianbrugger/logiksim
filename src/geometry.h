#ifndef LOGIKSIM_GEOMETRY_H
#define LOGIKSIM_GEOMETRY_H

#include "vocabulary.h"

#include <tuple>

namespace logicsim {
auto is_horizontal(line_t line) noexcept -> bool;

auto is_vertical(line_t line) noexcept -> bool;

// order points within lines
auto order_points(line_t line) noexcept -> line_t;

// order lines and points within lines
auto order_points(const line_t line0, const line_t line1) noexcept
    -> std::tuple<line_t, line_t>;

// fast distance for horitonal or vertical lines
auto distance(line_t line) -> int;

/*
constexpr auto is_horizontal(line_t line) noexcept -> bool {
    return line.p0.y == line.p1.y;
}

constexpr auto is_vertical(line_t line) noexcept -> bool {
    return line.p0.x == line.p1.x;
}

// order points within lines
constexpr auto order_points(line_t line) noexcept {
    auto [p0, p1] = sorted(line.p0, line.p1);
    return line_t {p0, p1};
}

// order lines and points within lines
constexpr auto order_points(const line_t line0, const line_t line1) noexcept
    -> std::tuple<line_t, line_t> {
    auto a = order_points(line0);
    auto b = order_points(line1);

    if (a.p0 <= b.p0) {
        return std::tie(a, b);
    }
    return std::tie(b, a);
}

// fast distance for horitonal or vertical lines
inline auto distance(line_t line) -> int {
    auto dx = line.p1.x.value - line.p0.x.value;
    auto dy = line.p1.y.value - line.p0.y.value;

    // ensure enough precision, through promotion
    static_assert(sizeof(dx) > sizeof(line.p0.x.value));
    return std::abs((dx == 0) ? dy : dx);
}
*/

}  // namespace logicsim

#endif