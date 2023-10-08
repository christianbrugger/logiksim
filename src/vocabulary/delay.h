#ifndef LOGICSIM_VOCABULARY_DELAY_H
#define LOGICSIM_VOCABULARY_DELAY_H

#include "concept/integral.h"
#include "format/struct.h"
#include "safe_numeric.h"
#include "vocabulary/time_literal.h"

#include <chrono>
#include <compare>
#include <cstdint>
#include <stdexcept>

namespace logicsim {

/**
 * @brief: Specifies a time duration of the simulation time.
 */
struct delay_t {
    using rep = int64_t;
    using period = std::nano;

   private:
    using rep_safe = ls_safe<rep>;
    using value_type = std::chrono::duration<rep_safe, period>;
    value_type value;

   public:
    [[nodiscard]] explicit constexpr delay_t() noexcept;
    [[nodiscard]] explicit constexpr delay_t(
        std::chrono::duration<rep, period> delay) noexcept;

    // returns safe_numerics time type
    [[nodiscard]] constexpr auto safe_value() const noexcept -> value_type;
    // returns nanosecond count
    [[nodiscard]] constexpr auto count_ns() const noexcept -> rep;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const delay_t &other) const -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const delay_t &other) const
        -> std::strong_ordering;

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

[[nodiscard]] constexpr auto operator+(const delay_t &left, const delay_t &right)
    -> delay_t;
[[nodiscard]] constexpr auto operator-(const delay_t &left, const delay_t &right)
    -> delay_t;
// int
[[nodiscard]] constexpr auto operator*(const delay_t &left, const int &right) -> delay_t;
[[nodiscard]] constexpr auto operator/(const delay_t &left, const int &right) -> delay_t;
// symmetric
[[nodiscard]] constexpr auto operator*(const int &left, const delay_t &right) -> delay_t;
[[nodiscard]] constexpr auto operator/(const int &left, const delay_t &right) -> delay_t;

//
// Implementation
//

constexpr delay_t::delay_t() noexcept : value {rep_safe {0}} {};

constexpr delay_t::delay_t(std::chrono::duration<rep, period> delay) noexcept
    : value {rep_safe {delay.count()}} {};

constexpr auto delay_t::safe_value() const noexcept -> value_type {
    return value;
}

constexpr auto delay_t::count_ns() const noexcept -> rep {
    return value.count();
}

constexpr auto delay_t::operator<=>(const delay_t &other) const -> std::strong_ordering {
    const auto a = rep {value.count()};
    const auto b = rep {other.value.count()};
    return a <=> b;
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
    value = value + right.value;
    return *this;
}

constexpr auto delay_t::operator-=(const delay_t &right) -> delay_t & {
    value = value - right.value;
    return *this;
}

constexpr auto delay_t::operator*=(const int &right) -> delay_t & {
    value = value_type {rep_safe {value.count() * rep_safe {right}}};
    return *this;
}

constexpr auto delay_t::operator/=(const int &right) -> delay_t & {
    value = value_type {rep_safe {value.count() / rep_safe {right}}};
    return *this;
}

constexpr auto delay_t::operator+() const -> delay_t {
    return *this;
}

constexpr auto delay_t::operator-() const -> delay_t {
    auto result = delay_t {};
    result.value = -value;
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

constexpr auto operator/(const int &left, const delay_t &right) -> delay_t {
    return operator/(right, left);
}

}  // namespace logicsim

#endif
