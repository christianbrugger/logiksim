#ifndef LOGICSIM_VOCABULARY_DELAY_H
#define LOGICSIM_VOCABULARY_DELAY_H

#include "core/algorithm/numeric.h"
#include "core/format/struct.h"
#include "core/vocabulary/time_literal.h"

#include <chrono>
#include <compare>
#include <cstdint>

namespace logicsim {

/**
 * @brief: Specifies a time duration of the simulation time.
 */
struct delay_t {
    using rep = int64_t;
    using period = std::nano;

    using value_type = std::chrono::duration<rep, period>;
    value_type value;

   public:
    [[nodiscard]] explicit constexpr delay_t() noexcept;
    [[nodiscard]] explicit constexpr delay_t(value_type delay) noexcept;

    // returns nanosecond count
    [[nodiscard]] constexpr auto count_ns() const noexcept -> rep;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const delay_t &other) const -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const delay_t &other) const
        -> std::strong_ordering = default;

    [[nodiscard]] static constexpr auto zero() noexcept -> delay_t;
    [[nodiscard]] static constexpr auto epsilon() noexcept -> delay_t;
    [[nodiscard]] static constexpr auto min() noexcept -> delay_t;
    [[nodiscard]] static constexpr auto max() noexcept -> delay_t;

    constexpr auto operator+=(const delay_t &right) -> delay_t &;
    constexpr auto operator-=(const delay_t &right) -> delay_t &;

    constexpr auto operator*=(const int &right) -> delay_t &;
    constexpr auto operator/=(const int &right) -> delay_t &;

    [[nodiscard]] constexpr auto operator+() const -> delay_t;
    [[nodiscard]] constexpr auto operator-() const -> delay_t;
};

static_assert(std::is_trivially_copyable_v<delay_t>);
static_assert(std::is_trivially_copy_constructible_v<delay_t>);
static_assert(std::is_trivially_copy_assignable_v<delay_t>);

[[nodiscard]] constexpr auto operator+(const delay_t &left,
                                       const delay_t &right) -> delay_t;
[[nodiscard]] constexpr auto operator-(const delay_t &left,
                                       const delay_t &right) -> delay_t;
// int
[[nodiscard]] constexpr auto operator*(const delay_t &left, const int &right) -> delay_t;
[[nodiscard]] constexpr auto operator/(const delay_t &left, const int &right) -> delay_t;
// symmetric
[[nodiscard]] constexpr auto operator*(const int &left, const delay_t &right) -> delay_t;

//
// Implementation
//

constexpr delay_t::delay_t() noexcept : value {rep {0}} {};

constexpr delay_t::delay_t(value_type delay) noexcept : value {delay} {};

constexpr auto delay_t::count_ns() const noexcept -> rep {
    return value.count();
}

constexpr auto delay_t::zero() noexcept -> delay_t {
    auto result = delay_t {};
    result.value = value_type::zero();
    return result;
};

constexpr auto delay_t::epsilon() noexcept -> delay_t {
    constexpr auto epsilon = ++value_type::zero();
    auto result = delay_t {};
    result.value = epsilon;
    return result;
};

constexpr auto delay_t::min() noexcept -> delay_t {
    auto result = delay_t {};
    result.value = value_type::min();
    return result;
};

constexpr auto delay_t::max() noexcept -> delay_t {
    auto result = delay_t {};
    result.value = value_type::max();
    return result;
};

constexpr auto delay_t::operator+=(const delay_t &right) -> delay_t & {
    value = value_type {checked_add(value.count(), right.value.count())};
    return *this;
}

constexpr auto delay_t::operator-=(const delay_t &right) -> delay_t & {
    value = value_type {checked_sub(value.count(), right.value.count())};
    return *this;
}

constexpr auto delay_t::operator*=(const int &right) -> delay_t & {
    value = value_type {checked_mul(value.count(), rep {right})};
    return *this;
}

constexpr auto delay_t::operator/=(const int &right) -> delay_t & {
    value = value_type {checked_div(value.count(), rep {right})};
    return *this;
}

constexpr auto delay_t::operator+() const -> delay_t {
    return *this;
}

constexpr auto delay_t::operator-() const -> delay_t {
    auto result = delay_t {};
    result.value = value_type {checked_neg(value.count())};
    return result;
}

//
// Free functions
//

constexpr auto operator+(const delay_t &left, const delay_t &right) -> delay_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const delay_t &left, const delay_t &right) -> delay_t {
    auto result = left;
    result -= right;
    return result;
}

constexpr auto operator*(const delay_t &left, const int &right) -> delay_t {
    auto result = left;
    result *= right;
    return result;
}

constexpr auto operator/(const delay_t &left, const int &right) -> delay_t {
    auto result = left;
    result /= right;
    return result;
}

constexpr auto operator*(const int &left, const delay_t &right) -> delay_t {
    return operator*(right, left);
}

}  // namespace logicsim

#endif
