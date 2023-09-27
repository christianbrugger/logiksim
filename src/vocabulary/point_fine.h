#ifndef LOGICSIM_VOCABULARY_POINT_FINE_H
#define LOGICSIM_VOCABULARY_POINT_FINE_H

#include "vocabulary/grid.h"
#include "vocabulary/grid_fine.h"
#include "vocabulary/point.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A continuous 2-d position on the grid.
 */
struct point_fine_t {
    grid_fine_t x;
    grid_fine_t y;

    point_fine_t() = default;

    [[nodiscard]] constexpr point_fine_t(grid_fine_t x_, grid_fine_t y_) noexcept
        : x {x_}, y {y_} {}

    [[nodiscard]] explicit constexpr point_fine_t(grid_t x_, grid_t y_) noexcept
        : x {grid_fine_t {x_}}, y {grid_fine_t {y_}} {}

    [[nodiscard]] explicit constexpr point_fine_t(point_t point) noexcept
        : x {grid_fine_t {point.x}}, y {grid_fine_t {point.y}} {}

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const point_fine_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const point_fine_t &other) const = default;

    [[nodiscard]] constexpr auto operator+(point_fine_t other) const -> point_fine_t {
        return point_fine_t {x + other.x, y + other.y};
    }

    [[nodiscard]] constexpr auto operator-(point_fine_t other) const -> point_fine_t {
        return point_fine_t {x - other.x, y - other.y};
    }

    constexpr auto operator+=(point_fine_t other) -> point_fine_t & {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr auto operator-=(point_fine_t other) -> point_fine_t & {
        x -= other.x;
        y -= other.y;
        return *this;
    }
};

static_assert(std::is_trivial<point_fine_t>::value);

/**
 * @brief: Returns if the line from p0 to p1 is horizontal or vertical.
 *
 * Returns false for zero length lines.
 */
constexpr auto is_orthogonal_line(point_fine_t p0, point_fine_t p1) noexcept -> bool {
    return (p0.x == p1.x) ^ (p0.y == p1.y);
}

}  // namespace logicsim

#endif
