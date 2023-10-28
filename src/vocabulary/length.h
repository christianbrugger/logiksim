#ifndef LOGICSIM_VOCABULARY_LENGTH_H
#define LOGICSIM_VOCABULARY_LENGTH_H

#include "format/struct.h"
#include "type_trait/safe_difference_type.h"
#include "vocabulary/grid.h"

#include <compare>
#include <limits>

namespace logicsim {

struct length_t {
    using value_type = grid_t::difference_type;
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const length_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const length_t &other) const = default;

    constexpr auto operator+=(const length_t &right) -> length_t &;
    constexpr auto operator-=(const length_t &right) -> length_t &;
};

static_assert(std::is_trivial_v<length_t>);
static_assert(std::is_trivially_constructible_v<length_t>);
static_assert(std::is_trivially_copyable_v<length_t>);
static_assert(std::is_trivially_copy_assignable_v<length_t>);

[[nodiscard]] constexpr auto operator+(const length_t &left, const length_t &right)
    -> length_t;
[[nodiscard]] constexpr auto operator-(const length_t &left, const length_t &right)
    -> length_t;


//
// Implementation
//

constexpr auto length_t::operator+=(const length_t &right) -> length_t & {
    using T = length_t::difference_type;
    auto result = T {value} + T {right.value};

    static_assert(sizeof(result) > sizeof(value));
    value = gsl::narrow<length_t::value_type>(result);

    return *this;
}

constexpr auto length_t::operator-=(const length_t &right) -> length_t & {
    using T = length_t::difference_type;
    auto result = T {value} - T {right.value};

    static_assert(sizeof(result) > sizeof(value));
    value = gsl::narrow<length_t::value_type>(result);

    return *this;
}

//
// Free Functions
//


constexpr auto operator+(const length_t &left, const length_t &right) -> length_t {
    auto result = left;
    result += right;
    return result;
}

constexpr auto operator-(const length_t &left, const length_t &right) -> length_t {
    auto result = left;
    result -= right;
    return result;
}

}  // namespace logicsim

#endif
