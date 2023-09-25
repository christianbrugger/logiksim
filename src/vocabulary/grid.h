#ifndef LOGICSIM_VOCABULARY_GRID_H
#define LOGICSIM_VOCABULARY_GRID_H

#include "difference_type.h"
#include "format/struct.h"
#include "vocabulary/grid_fine.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: A discrete location on the grid in one dimension.
 */
struct grid_t {
    using value_type = int16_t;
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    grid_t() = default;

    [[nodiscard]] constexpr grid_t(value_type v) noexcept : value {v} {};

    [[nodiscard]] constexpr explicit grid_t(int v) noexcept
        : value {gsl::narrow<value_type>(v)} {};
    [[nodiscard]] constexpr explicit grid_t(unsigned int v) noexcept
        : value {gsl::narrow<value_type>(v)} {};

    [[nodiscard]] constexpr explicit grid_t(long v) noexcept
        : value {gsl::narrow<value_type>(v)} {};
    [[nodiscard]] constexpr explicit grid_t(unsigned long v) noexcept
        : value {gsl::narrow<value_type>(v)} {};

    [[nodiscard]] constexpr explicit grid_t(long long v) noexcept
        : value {gsl::narrow<value_type>(v)} {};
    [[nodiscard]] constexpr explicit grid_t(unsigned long long v) noexcept
        : value {gsl::narrow<value_type>(v)} {};

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const grid_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const grid_t &other) const = default;

    [[nodiscard]] static constexpr auto min() noexcept {
        return std::numeric_limits<value_type>::min();
    };

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    // arithmetic (overflow is checked)

    [[nodiscard]] constexpr auto operator+(grid_t other) const -> grid_t {
        auto result = value + other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<grid_t::value_type>(result)};
    }

    [[nodiscard]] constexpr auto operator-(grid_t other) const -> grid_t {
        auto result = value - other.value;

        static_assert(sizeof(result) > sizeof(value));
        return {gsl::narrow<grid_t::value_type>(result)};
    }

    constexpr auto operator++() -> grid_t & {
        return *this = *this + grid_t {1};
    }

    constexpr auto operator++(int) -> grid_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }

    // conversions

    [[nodiscard]] explicit constexpr operator int() const noexcept {
        return static_cast<int>(value);
    }

    [[nodiscard]] explicit constexpr operator grid_fine_t() const noexcept {
        return static_cast<grid_fine_t>(value);
    }

    [[nodiscard]] friend constexpr auto operator*(grid_t a, grid_fine_t b)
        -> grid_fine_t {
        return static_cast<grid_fine_t>(a) * b;
    }
};

static_assert(std::is_trivial<grid_t>::value);
static_assert(std::is_standard_layout<grid_t>::value);
static_assert(std::is_nothrow_default_constructible<grid_t>::value);

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::grid_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::grid_t &obj) const noexcept
        -> uint64_t {
        return logicsim::wyhash(obj.value);
    }
};

#endif
