#ifndef LOGICSIM_VOCABULARY_GRID_H
#define LOGICSIM_VOCABULARY_GRID_H

#include "difference_type.h"
#include "format/struct.h"
#include "vocabulary/grid_fine.h"
#include "wyhash.h"

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
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    grid_t() = default;
    [[nodiscard]] constexpr grid_t(value_type v) noexcept;
    [[nodiscard]] constexpr explicit grid_t(int v);
    [[nodiscard]] constexpr explicit grid_t(unsigned int v);
    [[nodiscard]] constexpr explicit grid_t(long v);
    [[nodiscard]] constexpr explicit grid_t(unsigned long v);
    [[nodiscard]] constexpr explicit grid_t(long long v);
    [[nodiscard]] constexpr explicit grid_t(unsigned long long v);

    [[nodiscard]] explicit constexpr operator int() const noexcept;
    [[nodiscard]] explicit constexpr operator grid_fine_t() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const grid_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const grid_t &other) const = default;

    [[nodiscard]] static constexpr auto min() noexcept;
    [[nodiscard]] static constexpr auto max() noexcept;

    // arithmetic (overflow is checked)

    constexpr auto operator+=(const grid_t &other) -> grid_t &;
    constexpr auto operator-=(const grid_t &other) -> grid_t &;

    constexpr auto operator*=(const int &right) -> grid_t &;
    constexpr auto operator/=(const int &right) -> grid_t &;

    constexpr auto operator++() -> grid_t &;
    constexpr auto operator++(int) -> grid_t;

    constexpr auto operator--() -> grid_t &;
    constexpr auto operator--(int) -> grid_t;
};

[[nodiscard]] constexpr auto operator+(const grid_t &left, const grid_t &right) -> grid_t;
[[nodiscard]] constexpr auto operator-(const grid_t &left, const grid_t &right) -> grid_t;
// int
[[nodiscard]] constexpr auto operator*(const grid_t &left, const int &right) -> grid_t;
[[nodiscard]] constexpr auto operator/(const grid_t &left, const int &right) -> grid_t;
// TODO once grid_fine_t is a strong type, and int is not implicitely convertible to it
[[nodiscard]] constexpr auto operator*(const int &left, const grid_t &right) -> grid_t;
[[nodiscard]] constexpr auto operator/(const int &left, const grid_t &right) -> grid_t;

// grid_fine_t  // TODO where is this needed??
[[nodiscard]] constexpr auto operator*(const grid_t &left, const grid_fine_t &right)
    -> grid_fine_t;
// TODO once grid_fine_t is a strong type, and int is not implicitely convertible to it
[[nodiscard]] constexpr auto operator*(const grid_fine_t &left, const grid_t &right)
    -> grid_fine_t;

static_assert(std::is_trivial<grid_t>::value);
static_assert(std::is_standard_layout<grid_t>::value);
static_assert(std::is_nothrow_default_constructible<grid_t>::value);

//
// Implementation
//

constexpr grid_t::grid_t(value_type v) noexcept : value {v} {};

constexpr grid_t::grid_t(int v) : value {gsl::narrow<value_type>(v)} {};

constexpr grid_t::grid_t(unsigned int v) : value {gsl::narrow<value_type>(v)} {};

constexpr grid_t::grid_t(long v) : value {gsl::narrow<value_type>(v)} {};

constexpr grid_t::grid_t(unsigned long v) : value {gsl::narrow<value_type>(v)} {};

constexpr grid_t::grid_t(long long v) : value {gsl::narrow<value_type>(v)} {};

constexpr grid_t::grid_t(unsigned long long v) : value {gsl::narrow<value_type>(v)} {};

constexpr auto grid_t::min() noexcept {
    return std::numeric_limits<value_type>::min();
};

constexpr auto grid_t::max() noexcept {
    return std::numeric_limits<value_type>::max();
};

constexpr auto grid_t::operator+=(const grid_t &other) -> grid_t & {
    auto result = value + other.value;

    static_assert(sizeof(result) > sizeof(value));
    value = gsl::narrow<grid_t::value_type>(result);

    return *this;
}

constexpr auto grid_t::operator-=(const grid_t &other) -> grid_t & {
    auto result = value - other.value;

    static_assert(sizeof(result) > sizeof(value));
    value = gsl::narrow<grid_t::value_type>(result);

    return *this;
}

constexpr auto grid_t::operator*=(const int &right) -> grid_t & {
    auto result = int64_t {value} * int64_t {right};

    static_assert(sizeof(result) > sizeof(value) && sizeof(result) > sizeof(right));
    value = gsl::narrow<grid_t::value_type>(result);

    return *this;
}

constexpr auto grid_t::operator/=(const int &right) -> grid_t & {
    if (right == 0) {
        throw std::runtime_error("division by zero");
    }
    auto result = int64_t {value} / int64_t {right};

    static_assert(sizeof(result) > sizeof(value) && sizeof(result) > sizeof(right));
    value = gsl::narrow<grid_t::value_type>(result);

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

constexpr grid_t::operator int() const noexcept {
    return static_cast<int>(value);
}

constexpr grid_t::operator grid_fine_t() const noexcept {
    return static_cast<grid_fine_t>(value);
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

constexpr auto operator/(const int &left, const grid_t &right) -> grid_t {
    return operator/(right, left);
}

constexpr auto operator*(const grid_t &left, const grid_fine_t &right) -> grid_fine_t {
    return static_cast<grid_fine_t>(left) * right;
}

[[nodiscard]] constexpr auto operator*(const grid_fine_t &left, const grid_t &right)
    -> grid_fine_t {
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
