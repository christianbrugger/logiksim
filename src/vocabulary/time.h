#ifndef LOGICSIM_VOCABULARY_TIME_H
#define LOGICSIM_VOCABULARY_TIME_H

#include "format/struct.h"
#include "safe_numeric.h"
#include "vocabulary/delay.h"
#include "vocabulary/time_literal.h"

#include <chrono>
#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Specifies the current simulation time point
 */
struct time_t {
    using rep = int64_t;
    using period = std::nano;

   private:
    using rep_safe = ls_safe<rep>;
    using value_type = std::chrono::duration<rep_safe, period>;
    value_type value;

   public:
    [[nodiscard]] explicit constexpr time_t() noexcept;
    [[nodiscard]] explicit constexpr time_t(
        std::chrono::duration<rep, period> time) noexcept;

    // returns safe_numerics time type
    [[nodiscard]] constexpr auto safe_value() const noexcept -> value_type;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const time_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const time_t &other) const
        -> std::strong_ordering = default;

    [[nodiscard]] static constexpr auto zero() noexcept -> time_t;
    [[nodiscard]] static constexpr auto epsilon() noexcept -> delay_t;
    [[nodiscard]] static constexpr auto min() noexcept -> time_t;
    [[nodiscard]] static constexpr auto max() noexcept -> time_t;

    constexpr auto operator+=(const delay_t &right) -> time_t &;
    constexpr auto operator-=(const delay_t &right) -> time_t &;
};

static_assert(std::is_trivially_copyable_v<time_t>);
static_assert(std::is_trivially_copy_constructible_v<time_t>);
static_assert(std::is_trivially_copy_assignable_v<time_t>);

[[nodiscard]] constexpr auto operator-(const time_t &left, const time_t &right)
    -> delay_t;
// int
[[nodiscard]] constexpr auto operator+(const time_t &left, const delay_t &right)
    -> time_t;
[[nodiscard]] constexpr auto operator-(const time_t &left, const delay_t &right)
    -> time_t;
// symmetric
[[nodiscard]] constexpr auto operator+(const delay_t &left, const time_t &right)
    -> time_t;
[[nodiscard]] constexpr auto operator-(const delay_t &left, const time_t &right)
    -> time_t;

//
// Implementation
//

constexpr time_t::time_t() noexcept : value {rep_safe {0}} {};

constexpr time_t::time_t(std::chrono::duration<rep, period> time) noexcept
    : value {rep_safe {time.count()}} {};

constexpr auto time_t::safe_value() const noexcept -> value_type {
    return value;
}

constexpr auto time_t::zero() noexcept -> time_t {
    auto result = time_t {};
    result.value = value_type::zero();
    return result;
};

constexpr auto time_t::epsilon() noexcept -> delay_t {
    static_assert(std::ratio_equal_v<time_t::period, delay_t::period>);
    return delay_t::epsilon();
};

constexpr auto time_t::min() noexcept -> time_t {
    auto result = time_t {};
    result.value = value_type::min();
    return result;
};

constexpr auto time_t::max() noexcept -> time_t {
    auto result = time_t {};
    result.value = value_type::max();
    return result;
};

constexpr auto time_t::operator+=(const delay_t &right) -> time_t & {
    value += right.safe_value();
    return *this;
}

constexpr auto time_t::operator-=(const delay_t &right) -> time_t & {
    value -= right.safe_value();
    return *this;
}

//
// Free functions
//

[[nodiscard]] constexpr auto operator-(const time_t &left, const time_t &right)
    -> delay_t {
    using T = std::chrono::duration<time_t::rep, time_t::period>;
    const auto left_duration = T {time_t::rep {left.safe_value().count()}};
    const auto right_duration = T {time_t::rep {right.safe_value().count()}};

    return delay_t {left_duration} - delay_t {right_duration};
}

constexpr auto operator+(const time_t &left, const delay_t &right) -> time_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const time_t &left, const delay_t &right) -> time_t {
    auto result = left;
    result -= right;
    return result;
}

constexpr auto operator+(const delay_t &left, const time_t &right) -> time_t {
    return operator+(right, left);
}

constexpr auto operator-(const delay_t &left, const time_t &right) -> time_t {
    return operator-(right, left);
}

}  // namespace logicsim

#endif
