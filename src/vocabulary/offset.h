#ifndef LOGICSIM_VOCABULARY_OFFSET_H
#define LOGICSIM_VOCABULARY_OFFSET_H

#include "concept/explicitly_convertible.h"
#include "concept/integral.h"
#include "format/struct.h"
#include "type_trait/safe_difference_type.h"
#include "vocabulary/grid.h"

#include <gsl/gsl>

#include <compare>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A positive, discrete, 1-d offset on the grid.
 */
struct offset_t {
    using value_type = std::make_unsigned_t<grid_t::value_type>;
    static_assert(sizeof(offset_t::value_type) == sizeof(grid_t::value_type));
    value_type value {0};

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr offset_t() = default;
    [[nodiscard]] explicit constexpr offset_t(integral auto value_);

    [[nodiscard]] explicit constexpr operator int() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const offset_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const offset_t &other) const = default;

    [[nodiscard]] static constexpr auto min() noexcept -> offset_t;
    [[nodiscard]] static constexpr auto max() noexcept -> offset_t;

    constexpr auto operator+=(const offset_t &other) -> offset_t &;
    constexpr auto operator-=(const offset_t &other) -> offset_t &;

    constexpr auto operator*=(const int &right) -> offset_t &;
    constexpr auto operator/=(const int &right) -> offset_t &;

    constexpr auto operator++() -> offset_t &;
    constexpr auto operator++(int) -> offset_t;

    constexpr auto operator--() -> offset_t &;
    constexpr auto operator--(int) -> offset_t;
};

static_assert(std::is_trivially_copyable_v<offset_t>);
static_assert(std::is_trivially_copy_assignable_v<offset_t>);
static_assert(std::is_standard_layout_v<offset_t>);
static_assert(std::is_nothrow_default_constructible_v<offset_t>);
static_assert(explicitly_convertible_to<offset_t, offset_t::difference_type>);

[[nodiscard]] constexpr auto operator+(const offset_t &left, const offset_t &right)
    -> offset_t;
[[nodiscard]] constexpr auto operator-(const offset_t &left, const offset_t &right)
    -> offset_t;
// int
[[nodiscard]] constexpr auto operator*(const offset_t &left, const int &right)
    -> offset_t;
[[nodiscard]] constexpr auto operator/(const offset_t &left, const int &right)
    -> offset_t;
// symmetric
[[nodiscard]] constexpr auto operator*(const int &left, const offset_t &right)
    -> offset_t;

//
// Concepts
//

/**
 * @brief: Any type that is explicitely convertible to offset_t
 */
template <typename T>
concept offset_like = explicitly_convertible_to<T, offset_t>;

//
// Implementation
//

constexpr offset_t::offset_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr offset_t::operator int() const noexcept {
    return int {value};
}

constexpr auto offset_t::min() noexcept -> offset_t {
    return offset_t {std::numeric_limits<value_type>::min()};
};

constexpr auto offset_t::max() noexcept -> offset_t {
    return offset_t {std::numeric_limits<value_type>::max()};
};

constexpr auto offset_t::operator++() -> offset_t & {
    *this += offset_t {1};
    return *this;
}

constexpr auto offset_t::operator++(int) -> offset_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

constexpr auto offset_t::operator--() -> offset_t & {
    *this -= offset_t {1};
    return *this;
}

constexpr auto offset_t::operator--(int) -> offset_t {
    auto tmp = *this;
    operator--();
    return tmp;
}

constexpr auto offset_t::operator+=(const offset_t &other) -> offset_t & {
    auto result = value + other.value;

    static_assert(sizeof(result) > sizeof(value));
    value = gsl::narrow<offset_t::value_type>(result);

    return *this;
}

constexpr auto offset_t::operator-=(const offset_t &other) -> offset_t & {
    auto result = value - other.value;

    static_assert(sizeof(result) > sizeof(value));
    value = gsl::narrow<offset_t::value_type>(result);

    return *this;
}

constexpr auto offset_t::operator*=(const int &right) -> offset_t & {
    auto result = int64_t {value} * int64_t {right};

    static_assert(sizeof(result) > sizeof(value));
    static_assert(sizeof(result) > sizeof(right));

    value = gsl::narrow<value_type>(result);
    return *this;
}

constexpr auto offset_t::operator/=(const int &right) -> offset_t & {
    if (right == 0) {
        throw std::runtime_error("division by zero");
    }
    auto result = int64_t {value} / int64_t {right};

    static_assert(sizeof(result) > sizeof(value));
    static_assert(sizeof(result) > sizeof(right));

    value = gsl::narrow<value_type>(result);
    return *this;
}

//
// free functions
//

[[nodiscard]] constexpr auto operator+(const offset_t &left, const offset_t &right)
    -> offset_t {
    auto result = left;
    result += right;
    return result;
}

[[nodiscard]] constexpr auto operator-(const offset_t &left, const offset_t &right)
    -> offset_t {
    auto result = left;
    result -= right;
    return result;
}

constexpr auto operator*(const offset_t &left, const int &right) -> offset_t {
    auto result = left;
    result *= right;
    return result;
}

constexpr auto operator/(const offset_t &left, const int &right) -> offset_t {
    auto result = left;
    result /= right;
    return result;
}

constexpr auto operator*(const int &left, const offset_t &right) -> offset_t {
    return operator*(right, left);
}

}  // namespace logicsim

#endif
