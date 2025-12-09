#ifndef LOGICSIM_VOCABULARY_TIME_H
#define LOGICSIM_VOCABULARY_TIME_H

#include "core/algorithm/numeric.h"
#include "core/format/struct.h"
#include "core/vocabulary/delay.h"
#include "core/vocabulary/time_literal.h"

#include <chrono>
#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Specifies the current simulation time point
 */
struct time_t {
   public:
    using rep = int64_t;
    using period = std::nano;

    using value_type = std::chrono::duration<int64_t, period>;
    value_type value;

   public:
    [[nodiscard]] explicit constexpr time_t() noexcept;
    [[nodiscard]] explicit constexpr time_t(value_type time) noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] constexpr auto operator==(const time_t &other) const -> bool = default;
    [[nodiscard]] constexpr auto operator<=>(const time_t &other) const
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

[[nodiscard]] constexpr auto operator-(const time_t &left,
                                       const time_t &right) -> delay_t;
// int
[[nodiscard]] constexpr auto operator+(const time_t &left,
                                       const delay_t &right) -> time_t;
[[nodiscard]] constexpr auto operator-(const time_t &left,
                                       const delay_t &right) -> time_t;
// symmetric
[[nodiscard]] constexpr auto operator+(const delay_t &left,
                                       const time_t &right) -> time_t;

//
// Implementation
//

constexpr time_t::time_t() noexcept : value {rep {0}} {};

constexpr time_t::time_t(value_type time) noexcept : value {time} {};

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
    static_assert(std::same_as<rep, delay_t::rep>);
    static_assert(std::ratio_equal_v<period, delay_t::period>);

    value = value_type {checked_add(value.count(), right.value.count())};
    return *this;
}

constexpr auto time_t::operator-=(const delay_t &right) -> time_t & {
    static_assert(std::same_as<rep, delay_t::rep>);
    static_assert(std::ratio_equal_v<period, delay_t::period>);

    value = value_type {checked_sub(value.count(), right.value.count())};
    return *this;
}

//
// Free functions
//

[[nodiscard]] constexpr auto operator-(const time_t &left,
                                       const time_t &right) -> delay_t {
    return delay_t {left.value} - delay_t {right.value};
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

}  // namespace logicsim

#endif
