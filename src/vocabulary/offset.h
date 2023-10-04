#ifndef LOGICSIM_VOCABULARY_OFFSET_H
#define LOGICSIM_VOCABULARY_OFFSET_H

#include "format/struct.h"
#include "type_trait/safe_difference_type.h"
#include "vocabulary/grid.h"

#include <gsl/gsl>

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A positive, discrete, 1-d offset on the grid.
 */
struct offset_t {
    using value_type = std::make_unsigned_t<grid_t::value_type>;
    static_assert(sizeof(offset_t::value_type) == sizeof(grid_t::value_type));

    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const offset_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const offset_t &other) const = default;

    constexpr auto operator+=(const offset_t &other) -> offset_t &;
    constexpr auto operator-=(const offset_t &other) -> offset_t &;

    constexpr auto operator++() -> offset_t &;
    constexpr auto operator++(int) -> offset_t;

    constexpr auto operator--() -> offset_t &;
    constexpr auto operator--(int) -> offset_t;
};

static_assert(std::is_aggregate<offset_t>::value);
static_assert(std::is_trivial<offset_t>::value);
static_assert(std::is_standard_layout<offset_t>::value);
static_assert(std::is_nothrow_default_constructible<offset_t>::value);

[[nodiscard]] constexpr auto operator+(const offset_t &left, const offset_t &right)
    -> offset_t;
[[nodiscard]] constexpr auto operator-(const offset_t &left, const offset_t &right)
    -> offset_t;

//
// Implementation
//

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

}  // namespace logicsim

#endif
