#ifndef LOGICSIM_VOCABULARY_OFFSET_H
#define LOGICSIM_VOCABULARY_OFFSET_H

#include "difference_type.h"
#include "format/struct.h"
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

    constexpr auto operator++() -> offset_t & {
        return *this = *this + offset_t {1};
    }

    constexpr auto operator++(int) -> offset_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }

    constexpr auto operator--() -> offset_t & {
        return *this = *this - offset_t {1};
    }

    constexpr auto operator--(int) -> offset_t {
        auto tmp = *this;
        operator--();
        return tmp;
    }

    [[nodiscard]] constexpr auto operator+(offset_t other) const -> offset_t {
        auto result = value + other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<offset_t::value_type>(result)};
    }

    [[nodiscard]] constexpr auto operator-(offset_t other) const -> offset_t {
        auto result = value - other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<offset_t::value_type>(result)};
    }
};

static_assert(std::is_aggregate<offset_t>::value);
static_assert(std::is_trivial<offset_t>::value);
static_assert(std::is_standard_layout<offset_t>::value);
static_assert(std::is_nothrow_default_constructible<offset_t>::value);

}  // namespace logicsim

#endif
