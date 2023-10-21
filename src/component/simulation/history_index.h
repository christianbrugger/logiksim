#ifndef LOGICSIM_COMPONENT_SIMULATION_HISTORY_INDEX_H
#define LOGICSIM_COMPONENT_SIMULATION_HISTORY_INDEX_H

#include "concept/integral.h"
#include "format/struct.h"
#include "safe_numeric.h"

#include <gsl/gsl>

#include <compare>
#include <limits>
#include <stdexcept>
#include <string>

namespace logicsim {

/**
 * @brief: Identifier to a history-entry in a history-buffer.
 */
struct history_index_t {
    using value_type = std::size_t;
    value_type value;

    using difference_type = std::ptrdiff_t;

    [[nodiscard]] explicit constexpr operator std::size_t() const;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr static auto max() -> history_index_t;

    [[nodiscard]] auto operator==(const history_index_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const history_index_t &other) const = default;

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

constexpr history_index_t::operator std::size_t() const {
    return value;
}

constexpr auto history_index_t::max() -> history_index_t {
    return history_index_t {std::numeric_limits<value_type>::max()};
}

constexpr auto history_index_t::operator++() -> history_index_t & {
    if (value == std::numeric_limits<value_type>::max()) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto history_index_t::operator++(int) -> history_index_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

constexpr auto history_index_t::operator+=(const integral auto &right)
    -> history_index_t & {
    using diff_t = history_index_t::difference_type;
    using safe_t = ls_safe<diff_t>;

    const auto res = diff_t {safe_t {value} + safe_t {right}};
    value = gsl::narrow<std::size_t>(res);

    return *this;
}

constexpr auto history_index_t::operator-=(const integral auto &right)
    -> history_index_t & {
    using diff_t = history_index_t::difference_type;
    using safe_t = ls_safe<diff_t>;

    const auto res = diff_t {safe_t {value} - safe_t {right}};
    value = gsl::narrow<std::size_t>(res);

    return *this;
}

//
// Free Functions
//

[[nodiscard]] constexpr auto operator-(const history_index_t &left,
                                       const history_index_t &right)
    -> history_index_t::difference_type {
    using diff_t = history_index_t::difference_type;
    using safe_t = ls_safe<diff_t>;

    return diff_t {safe_t {left.value} - safe_t {right.value}};
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

}  // namespace logicsim

#endif
