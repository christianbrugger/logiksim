#ifndef LOGICSIM_VOCABULARY_GRID_H
#define LOGICSIM_VOCABULARY_GRID_H

#include "core/algorithm/narrow_integral.h"
#include "core/concept/explicitly_convertible.h"
#include "core/concept/integral.h"
#include "core/format/struct.h"
#include "core/type_trait/safe_difference_type.h"
#include "core/wyhash.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <compare>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A discrete location on the grid in one dimension.
 */
struct grid_t {
    using value_type = int16_t;
    value_type value {0};

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr grid_t() = default;
    [[nodiscard]] explicit constexpr grid_t(integral auto value_);

    [[nodiscard]] explicit constexpr operator int() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const grid_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const grid_t &other) const = default;

    [[nodiscard]] static constexpr auto min() noexcept -> grid_t;
    [[nodiscard]] static constexpr auto max() noexcept -> grid_t;

    // arithmetic (overflow is checked)

    constexpr auto operator+=(const grid_t &right) -> grid_t &;
    constexpr auto operator-=(const grid_t &right) -> grid_t &;

    constexpr auto operator*=(const int &right) -> grid_t &;
    constexpr auto operator/=(const int &right) -> grid_t &;

    constexpr auto operator++() -> grid_t &;
    constexpr auto operator++(int) -> grid_t;

    constexpr auto operator--() -> grid_t &;
    constexpr auto operator--(int) -> grid_t;

    [[nodiscard]] constexpr auto operator+() const -> grid_t;
    [[nodiscard]] constexpr auto operator-() const -> grid_t;
};

static_assert(std::is_trivially_copyable_v<grid_t>);
static_assert(std::is_trivially_copy_assignable_v<grid_t>);
static_assert(explicitly_convertible_to<grid_t, grid_t::difference_type>);

[[nodiscard]] constexpr auto operator+(const grid_t &left, const grid_t &right) -> grid_t;
[[nodiscard]] constexpr auto operator-(const grid_t &left, const grid_t &right) -> grid_t;
// int
[[nodiscard]] constexpr auto operator*(const grid_t &left, const int &right) -> grid_t;
[[nodiscard]] constexpr auto operator/(const grid_t &left, const int &right) -> grid_t;
// symmetric
[[nodiscard]] constexpr auto operator*(const int &left, const grid_t &right) -> grid_t;

//
// Concepts
//

/**
 * @brief: Any type that is explicitely convertible to grid_t
 */
template <typename T>
concept grid_like = explicitly_convertible_to<T, grid_t>;

//
// Implementation
//

constexpr grid_t::grid_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr grid_t::operator int() const noexcept {
    return int {value};
}

constexpr auto grid_t::min() noexcept -> grid_t {
    return grid_t {std::numeric_limits<value_type>::min()};
};

constexpr auto grid_t::max() noexcept -> grid_t {
    return grid_t {std::numeric_limits<value_type>::max()};
};

constexpr auto grid_t::operator+=(const grid_t &right) -> grid_t & {
    auto result = value + right.value;

    static_assert(sizeof(result) > sizeof(value));
    value = gsl::narrow<grid_t::value_type>(result);

    return *this;
}

constexpr auto grid_t::operator-=(const grid_t &right) -> grid_t & {
    auto result = value - right.value;

    static_assert(sizeof(result) > sizeof(value));
    value = gsl::narrow<grid_t::value_type>(result);

    return *this;
}

constexpr auto grid_t::operator*=(const int &right) -> grid_t & {
    auto result = int64_t {value} * int64_t {right};

    static_assert(sizeof(result) > sizeof(value));
    static_assert(sizeof(result) > sizeof(right));

    value = gsl::narrow<value_type>(result);

    return *this;
}

constexpr auto grid_t::operator/=(const int &right) -> grid_t & {
    if (right == 0) {
        throw std::runtime_error("division by zero");
    }
    auto result = int64_t {value} / int64_t {right};

    static_assert(sizeof(result) > sizeof(value));
    static_assert(sizeof(result) > sizeof(right));

    value = gsl::narrow<value_type>(result);
    return *this;
}

constexpr auto grid_t::operator++() -> grid_t & {
    *this += grid_t {1};
    return *this;
}

constexpr auto grid_t::operator++(int) -> grid_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

constexpr auto grid_t::operator--() -> grid_t & {
    *this -= grid_t {1};
    return *this;
}

constexpr auto grid_t::operator--(int) -> grid_t {
    auto tmp = *this;
    operator--();
    return tmp;
}

constexpr auto grid_t::operator+() const -> grid_t {
    return *this;
}

constexpr auto grid_t::operator-() const -> grid_t {
    const auto res = -int {value};
    static_assert(sizeof(res) > sizeof(value));
    return grid_t {res};
}

constexpr auto operator+(const grid_t &left, const grid_t &right) -> grid_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const grid_t &left, const grid_t &right) -> grid_t {
    auto result = left;
    result -= right;
    return result;
}

constexpr auto operator*(const grid_t &left, const int &right) -> grid_t {
    auto result = left;
    result *= right;
    return result;
}

constexpr auto operator/(const grid_t &left, const int &right) -> grid_t {
    auto result = left;
    result /= right;
    return result;
}

constexpr auto operator*(const int &left, const grid_t &right) -> grid_t {
    return operator*(right, left);
}

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::grid_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::grid_t &obj) const noexcept
        -> uint64_t {
        return logicsim::wyhash(obj.value);
    }
};

#endif
