#ifndef LOGICSIM_VOCABULARY_POINT_H
#define LOGICSIM_VOCABULARY_POINT_H

#include "format/struct.h"
#include "vocabulary/grid.h"

#include <ankerl/unordered_dense.h>

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A discrete 2-d position on the grid.
 */
struct point_t {
    grid_t x {};
    grid_t y {};

    [[nodiscard]] explicit constexpr point_t() = default;
    [[nodiscard]] explicit constexpr point_t(grid_like auto x_, grid_like auto y_);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const point_t &other) const -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const point_t &other) const = default;

    constexpr auto operator+=(const point_t &other) -> point_t &;
    constexpr auto operator-=(const point_t &other) -> point_t &;
};

static_assert(std::is_trivially_copyable_v<point_t>);
static_assert(std::is_trivially_copy_assignable_v<point_t>);

[[nodiscard]] constexpr auto operator+(const point_t &left, const point_t &right)
    -> point_t;
[[nodiscard]] constexpr auto operator-(const point_t &left, const point_t &right)
    -> point_t;

/**
 * @brief: Returns if the line from p0 to p1 is horizontal or vertical.
 *
 * Returns false for zero length lines.
 */
constexpr auto is_orthogonal_line(point_t p0, point_t p1) noexcept -> bool;

//
// Implementation
//

constexpr point_t::point_t(grid_like auto x_, grid_like auto y_)
    : x {grid_t {x_}}, y {grid_t {y_}} {};

constexpr auto point_t::operator+=(const point_t &other) -> point_t & {
    x += other.x;
    y += other.y;
    return *this;
}

constexpr auto point_t::operator-=(const point_t &other) -> point_t & {
    x -= other.x;
    y -= other.y;
    return *this;
}

constexpr auto operator+(const point_t &left, const point_t &right) -> point_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const point_t &left, const point_t &right) -> point_t {
    auto result = left;
    result -= right;
    return result;
}

constexpr auto is_orthogonal_line(point_t p0, point_t p1) noexcept -> bool {
    return (p0.x == p1.x) != (p0.y == p1.y);
}
}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::point_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::point_t &obj) const noexcept
        -> uint64_t {
        using value_type = logicsim::point_t;
        using bit_type = uint32_t;

        static_assert(std::has_unique_object_representations_v<value_type>);
        static_assert(sizeof(value_type) == sizeof(bit_type));

        return logicsim::wyhash(std::bit_cast<bit_type>(obj));
    }
};

#endif
