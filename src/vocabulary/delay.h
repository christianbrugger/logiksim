#ifndef LOGICSIM_VOCABULARY_DELAY_H
#define LOGICSIM_VOCABULARY_DELAY_H

#include "concept/integral.h"
#include "format/struct.h"
#include "vocabulary/time_literal.h"

#include <chrono>
#include <compare>
#include <stdexcept>

namespace logicsim {

/**
 * @brief: Specifies a time duration of the simulation time.
 */
struct delay_t {
    // TODO use ls_safe
    // TODO check where .value is used
    using value_type = std::chrono::duration<int64_t, std::nano>;
    using rep = value_type::rep;
    value_type value;

    // TODO re-work constructors, same as time_t
    [[nodiscard]] explicit constexpr delay_t() noexcept = default;
    [[nodiscard]] explicit constexpr delay_t(
        std::chrono::duration<int64_t, std::nano> delay);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const delay_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const delay_t &other) const = default;

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

static_assert(std::is_trivial_v<delay_t>);

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

constexpr delay_t::delay_t(std::chrono::duration<int64_t, std::nano> delay)
    : value {delay} {
    if (value != delay) {
        throw std::runtime_error("delay cannot be represented.");
    }
};

constexpr auto delay_t::epsilon() noexcept -> delay_t {
    return delay_t {++value_type::zero()};
};

constexpr auto delay_t::min() noexcept -> delay_t {
    return delay_t {value_type::min()};
};

constexpr auto delay_t::max() noexcept -> delay_t {
    return delay_t {value_type::max()};
};

constexpr auto delay_t::operator+=(const delay_t &right) -> delay_t & {
    // TODO use ls_safe
    value += right.value;
    return *this;
}

constexpr auto delay_t::operator-=(const delay_t &right) -> delay_t & {
    // TODO use ls_safe
    value -= right.value;
    return *this;
}

constexpr auto delay_t::operator*=(const int &right) -> delay_t & {
    // TODO use ls_safe
    value *= right;
    return *this;
}

constexpr auto delay_t::operator/=(const int &right) -> delay_t & {
    // TODO use ls_safe
    value /= right;
    return *this;
}

constexpr auto delay_t::operator+() const -> delay_t {
    return *this;
}

constexpr auto delay_t::operator-() const -> delay_t {
    // TODO use ls_safe
    return delay_t {-value};
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
