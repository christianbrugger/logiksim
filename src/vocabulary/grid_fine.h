#ifndef LOGICSIM_VOCABULARY_GRID_FINE_H
#define LOGICSIM_VOCABULARY_GRID_FINE_H

#include "format/struct.h"
#include "vocabulary/grid.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A continuous location on the grid in one dimension.
 */
struct grid_fine_t {
    using value_type = double;
    value_type value {0.};

    [[nodiscard]] explicit constexpr grid_fine_t() = default;
    [[nodiscard]] explicit constexpr grid_fine_t(value_type v) noexcept;
    [[nodiscard]] explicit constexpr grid_fine_t(grid_t grid) noexcept;
    [[nodiscard]] explicit constexpr grid_fine_t(int v) noexcept;

    [[nodiscard]] explicit constexpr operator double() const noexcept;

    [[nodiscard]] auto format() const -> std::string;
    [[nodiscard]] constexpr auto operator==(const grid_fine_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const grid_fine_t &other) const = default;
    [[nodiscard]] constexpr auto operator==(const grid_t &other) const -> bool;
    [[nodiscard]] constexpr auto operator<=>(const grid_t &other) const
        -> std::partial_ordering;

    // @= grid_fine_t

    [[nodiscard]] constexpr auto operator+() const -> grid_fine_t;
    [[nodiscard]] constexpr auto operator-() const -> grid_fine_t;

    constexpr auto operator+=(const grid_fine_t &right) -> grid_fine_t &;
    constexpr auto operator-=(const grid_fine_t &right) -> grid_fine_t &;

    constexpr auto operator+=(const grid_t &right) -> grid_fine_t &;
    constexpr auto operator-=(const grid_t &right) -> grid_fine_t &;

    // @= grid_fine_t::value_type

    constexpr auto operator*=(const value_type &right) -> grid_fine_t &;
    constexpr auto operator/=(const value_type &right) -> grid_fine_t &;
};

static_assert(std::is_trivially_copyable_v<grid_fine_t>);
static_assert(std::is_trivially_copy_assignable_v<grid_fine_t>);

// grid_fine_t @ grid_fine_t

[[nodiscard]] constexpr auto operator+(const grid_fine_t &left,
                                       const grid_fine_t &right) -> grid_fine_t;
[[nodiscard]] constexpr auto operator-(const grid_fine_t &left,
                                       const grid_fine_t &right) -> grid_fine_t;

// grid_fine_t @ grid_fine_t::value_type
[[nodiscard]] constexpr auto operator*(
    const grid_fine_t &left, const grid_fine_t::value_type &right) -> grid_fine_t;
[[nodiscard]] constexpr auto operator*(const grid_fine_t::value_type &left,
                                       const grid_fine_t &right) -> grid_fine_t;

[[nodiscard]] constexpr auto operator/(
    const grid_fine_t &left, const grid_fine_t::value_type &right) -> grid_fine_t;

// grid_t @ grid_fine_t

[[nodiscard]] constexpr auto operator+(const grid_t &left,
                                       const grid_fine_t &right) -> grid_fine_t;
[[nodiscard]] constexpr auto operator+(const grid_fine_t &left,
                                       const grid_t &right) -> grid_fine_t;

[[nodiscard]] constexpr auto operator-(const grid_t &left,
                                       const grid_fine_t &right) -> grid_fine_t;
[[nodiscard]] constexpr auto operator-(const grid_fine_t &left,
                                       const grid_t &right) -> grid_fine_t;

// grid_t @ grid_fine_t::value_type
[[nodiscard]] constexpr auto operator*(
    const grid_t &left, const grid_fine_t::value_type &right) -> grid_fine_t;
[[nodiscard]] constexpr auto operator*(const grid_fine_t::value_type &left,
                                       const grid_t &right) -> grid_fine_t;

[[nodiscard]] constexpr auto operator/(
    const grid_t &left, const grid_fine_t::value_type &right) -> grid_fine_t;

//
// Concepts
//

/**
 * @brief: Any type that is explicitely convertible to grid_fine_t
 */
template <typename T>
concept grid_fine_like = explicitly_convertible_to<T, grid_fine_t>;

//
// Implementation
//

constexpr grid_fine_t::grid_fine_t(value_type v) noexcept : value {v} {}

constexpr grid_fine_t::grid_fine_t(grid_t grid) noexcept
    : value {static_cast<value_type>(grid.value)} {
    static_assert(sizeof(value_type) > sizeof(grid_t::value_type));
}

constexpr grid_fine_t::grid_fine_t(int v) noexcept : value {static_cast<value_type>(v)} {
    static_assert(sizeof(value_type) > sizeof(decltype(v)));
}

constexpr grid_fine_t::operator double() const noexcept {
    return double {value};
}

constexpr auto grid_fine_t::operator==(const grid_t &other) const -> bool {
    return *this == grid_fine_t {other};
}

constexpr auto grid_fine_t::operator<=>(const grid_t &other) const
    -> std::partial_ordering {
    return *this <=> grid_fine_t {other};
}

// @= grid_fine_t

constexpr auto grid_fine_t::operator+() const -> grid_fine_t {
    return grid_fine_t {+value};
}

constexpr auto grid_fine_t::operator-() const -> grid_fine_t {
    return grid_fine_t {-value};
}

constexpr auto grid_fine_t::operator+=(const grid_fine_t &right) -> grid_fine_t & {
    value += right.value;
    return *this;
}

constexpr auto grid_fine_t::operator-=(const grid_fine_t &right) -> grid_fine_t & {
    value -= right.value;
    return *this;
}

// @= grid_t

constexpr auto grid_fine_t::operator+=(const grid_t &right) -> grid_fine_t & {
    return operator+=(grid_fine_t {right});
}

constexpr auto grid_fine_t::operator-=(const grid_t &right) -> grid_fine_t & {
    return operator-=(grid_fine_t {right});
}

// @= value_type

constexpr auto grid_fine_t::operator*=(const value_type &right) -> grid_fine_t & {
    value *= right;
    return *this;
}

constexpr auto grid_fine_t::operator/=(const value_type &right) -> grid_fine_t & {
    if (right == value_type {0}) {
        throw std::runtime_error("division by zero");
    }
    value /= right;
    return *this;
}

// grid_fine_t @ grid_fine_t

constexpr auto operator+(const grid_fine_t &left,
                         const grid_fine_t &right) -> grid_fine_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const grid_fine_t &left,
                         const grid_fine_t &right) -> grid_fine_t {
    auto result = left;
    result -= right;
    return result;
}

// grid_fine_t @ grid_fine_t::value_type

constexpr auto operator*(const grid_fine_t &left,
                         const grid_fine_t::value_type &right) -> grid_fine_t {
    auto result = left;
    result *= right;
    return result;
}

constexpr auto operator*(const grid_fine_t::value_type &left,
                         const grid_fine_t &right) -> grid_fine_t {
    return operator*(right, left);
}

constexpr auto operator/(const grid_fine_t &left,
                         const grid_fine_t::value_type &right) -> grid_fine_t {
    auto result = left;
    result /= right;
    return result;
}

// grid_t @ grid_fine_t

[[nodiscard]] constexpr auto operator+(const grid_t &left,
                                       const grid_fine_t &right) -> grid_fine_t {
    return operator+(grid_fine_t {left}, right);
}

[[nodiscard]] constexpr auto operator+(const grid_fine_t &left,
                                       const grid_t &right) -> grid_fine_t {
    return operator+(left, grid_fine_t {right});
}

[[nodiscard]] constexpr auto operator-(const grid_t &left,
                                       const grid_fine_t &right) -> grid_fine_t {
    return operator-(grid_fine_t {left}, right);
}

[[nodiscard]] constexpr auto operator-(const grid_fine_t &left,
                                       const grid_t &right) -> grid_fine_t {
    return operator-(left, grid_fine_t {right});
}

// grid_t @ grid_fine_t::value_type

constexpr auto operator*(const grid_t &left,
                         const grid_fine_t::value_type &right) -> grid_fine_t {
    return operator*(grid_fine_t {left}, right);
}

constexpr auto operator*(const grid_fine_t::value_type &left,
                         const grid_t &right) -> grid_fine_t {
    return operator*(left, grid_fine_t {right});
}

constexpr auto operator/(const grid_t &left,
                         const grid_fine_t::value_type &right) -> grid_fine_t {
    return operator/(grid_fine_t {left}, right);
}

}  // namespace logicsim

#endif
