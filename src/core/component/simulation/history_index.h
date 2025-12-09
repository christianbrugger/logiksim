#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_INDEX_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_INDEX_H

#include "core/algorithm/numeric.h"
#include "core/concept/integral.h"
#include "core/format/struct.h"

#include <gsl/gsl>

#include <compare>
#include <limits>
#include <stdexcept>
#include <string>

namespace logicsim {

namespace simulation {

/**
 * @brief: Identifier to a history-entry in a history-buffer.
 *
 * Note indices can be negative as values can be extrapolated.
 */
struct history_index_t {
    using value_type = std::ptrdiff_t;
    value_type value;

    using difference_type = std::ptrdiff_t;

    [[nodiscard]] explicit constexpr operator std::size_t() const;
    [[nodiscard]] explicit constexpr operator std::ptrdiff_t() const;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr static auto min() -> history_index_t;
    [[nodiscard]] constexpr static auto max() -> history_index_t;

    [[nodiscard]] constexpr auto operator==(const history_index_t &other) const
        -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const history_index_t &other) const =
        default;

    constexpr auto operator++() -> history_index_t &;
    [[nodiscard]] constexpr auto operator++(int) -> history_index_t;

    constexpr auto operator+=(const integral auto &right) -> history_index_t &;
    constexpr auto operator-=(const integral auto &right) -> history_index_t &;
};

static_assert(std::is_trivial_v<history_index_t>);
static_assert(std::is_trivially_constructible_v<history_index_t>);
static_assert(std::is_trivially_copyable_v<history_index_t>);
static_assert(std::is_trivially_copy_assignable_v<history_index_t>);

// history_index_t @ history_index_t
[[nodiscard]] constexpr auto operator-(const history_index_t &left,
                                       const history_index_t &right)
    -> history_index_t::difference_type;

// history_index_t @ integral
[[nodiscard]] constexpr auto operator+(const history_index_t &left,
                                       const integral auto &right) -> history_index_t;
[[nodiscard]] constexpr auto operator-(const history_index_t &left,
                                       const integral auto &right) -> history_index_t;
// symmetric
[[nodiscard]] constexpr auto operator+(const integral auto &left,
                                       const history_index_t &right) -> history_index_t;

//
// Implementation
//

constexpr history_index_t::operator std::ptrdiff_t() const {
    return value;
}

constexpr history_index_t::operator std::size_t() const {
    return gsl::narrow<std::size_t>(value);
}

constexpr auto history_index_t::min() -> history_index_t {
    return history_index_t {std::numeric_limits<value_type>::min()};
}

constexpr auto history_index_t::max() -> history_index_t {
    return history_index_t {std::numeric_limits<value_type>::max()};
}

constexpr auto history_index_t::operator++() -> history_index_t & {
    value = checked_add(value, value_type {1});
    return *this;
}

constexpr auto history_index_t::operator++(int) -> history_index_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

constexpr auto history_index_t::operator+=(const integral auto &right)
    -> history_index_t & {
    static_assert(std::signed_integral<value_type>);

    value = checked_add(value, gsl::narrow<value_type>(right));
    return *this;
}

constexpr auto history_index_t::operator-=(const integral auto &right)
    -> history_index_t & {
    static_assert(std::signed_integral<value_type>);

    value = checked_sub(value, gsl::narrow<value_type>(right));
    return *this;
}

//
// Free Functions
//

[[nodiscard]] constexpr auto operator-(const history_index_t &left,
                                       const history_index_t &right)
    -> history_index_t::difference_type {
    return checked_sub(history_index_t::difference_type {left.value},
                       history_index_t::difference_type {right.value});
}

//
// history_index_t @ integral
//

[[nodiscard]] constexpr auto operator+(const history_index_t &left,
                                       const integral auto &right) -> history_index_t {
    auto result = left;
    result += right;
    return result;
}

[[nodiscard]] constexpr auto operator-(const history_index_t &left,
                                       const integral auto &right) -> history_index_t {
    auto result = left;
    result -= right;
    return result;
}

// symmetric
[[nodiscard]] constexpr auto operator+(const integral auto &left,
                                       const history_index_t &right) -> history_index_t {
    return operator+(right, left);
}

}  // namespace simulation

}  // namespace logicsim

#endif
