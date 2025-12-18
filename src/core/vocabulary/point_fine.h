#ifndef LOGICSIM_VOCABULARY_POINT_FINE_H
#define LOGICSIM_VOCABULARY_POINT_FINE_H

#include "core/concept/explicitly_convertible.h"
#include "core/format/struct.h"
#include "core/vocabulary/grid.h"
#include "core/vocabulary/grid_fine.h"
#include "core/vocabulary/point.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A continuous 2-d position on the grid.
 */
struct point_fine_t {
    grid_fine_t x {};
    grid_fine_t y {};

    [[nodiscard]] explicit constexpr point_fine_t() = default;
    [[nodiscard]] explicit constexpr point_fine_t(grid_fine_like auto x_,
                                                  grid_fine_like auto y_) noexcept;
    [[nodiscard]] explicit constexpr point_fine_t(point_t p) noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const point_fine_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const point_fine_t &other) const = default;

    constexpr auto operator+=(const point_fine_t &other) -> point_fine_t &;
    constexpr auto operator-=(const point_fine_t &other) -> point_fine_t &;
};

static_assert(std::is_trivially_copyable_v<point_fine_t>);
static_assert(std::is_trivially_copy_assignable_v<point_fine_t>);

[[nodiscard]] constexpr auto operator+(const point_fine_t &left,
                                       const point_fine_t &right) -> point_fine_t;
[[nodiscard]] constexpr auto operator-(const point_fine_t &left,
                                       const point_fine_t &right) -> point_fine_t;

// point_t
[[nodiscard]] constexpr auto operator+(const point_fine_t &left, const point_t &right)
    -> point_fine_t;
[[nodiscard]] constexpr auto operator-(const point_fine_t &left, const point_t &right)
    -> point_fine_t;
// symmetric
[[nodiscard]] constexpr auto operator+(const point_t &left, const point_fine_t &right)
    -> point_fine_t;
[[nodiscard]] constexpr auto operator-(const point_t &left, const point_fine_t &right)
    -> point_fine_t;

/**
 * @brief: Returns if the line from p0 to p1 is horizontal or vertical.
 *
 * Returns false for zero length lines.
 */
constexpr auto is_orthogonal_line(point_fine_t p0, point_fine_t p1) noexcept -> bool;

//
// Concepts
//

/**
 * @brief: Any type that is explicitely convertible to point_fine_t
 */
template <typename T>
concept point_fine_like = std::same_as<T, point_fine_t> || std::same_as<T, point_t>;

//
// Implementation
//

constexpr point_fine_t::point_fine_t(grid_fine_like auto x_,
                                     grid_fine_like auto y_) noexcept
    : x {grid_fine_t {x_}}, y {grid_fine_t {y_}} {}

constexpr point_fine_t::point_fine_t(point_t point) noexcept
    : point_fine_t {point.x, point.y} {}

constexpr auto point_fine_t::operator+=(const point_fine_t &other) -> point_fine_t & {
    x += other.x;
    y += other.y;
    return *this;
}

constexpr auto point_fine_t::operator-=(const point_fine_t &other) -> point_fine_t & {
    x -= other.x;
    y -= other.y;
    return *this;
}

constexpr auto operator+(const point_fine_t &left, const point_fine_t &right)
    -> point_fine_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const point_fine_t &left, const point_fine_t &right)
    -> point_fine_t {
    auto result = left;
    result -= right;
    return result;
}

constexpr auto operator+(const point_fine_t &left, const point_t &right) -> point_fine_t {
    return operator+(left, point_fine_t {right});
}

constexpr auto operator-(const point_fine_t &left, const point_t &right) -> point_fine_t {
    return operator-(left, point_fine_t {right});
}

constexpr auto operator+(const point_t &left, const point_fine_t &right) -> point_fine_t {
    return operator+(point_fine_t {left}, right);
}

constexpr auto operator-(const point_t &left, const point_fine_t &right) -> point_fine_t {
    return operator-(point_fine_t {left}, right);
}

constexpr auto is_orthogonal_line(point_fine_t p0, point_fine_t p1) noexcept -> bool {
    return (p0.x == p1.x) != (p0.y == p1.y);
}

}  // namespace logicsim

#endif
