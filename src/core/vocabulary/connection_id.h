#ifndef LOGICSIM_VOCABULARY_CONNECTION_ID_H
#define LOGICSIM_VOCABULARY_CONNECTION_ID_H

#include "core/algorithm/narrow_integral.h"
#include "core/concept/explicitly_convertible.h"
#include "core/concept/integral.h"
#include "core/format/struct.h"
#include "core/type_trait/safe_difference_type.h"

#include <gsl/gsl>

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifies an input or output of an unspecified circuit element.
 */
struct connection_id_t {
    using value_type = int16_t;
    // we expose the value, as the type has no invariant
    value_type value {-1};

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr connection_id_t() = default;
    [[nodiscard]] explicit constexpr connection_id_t(integral auto value_);

    [[nodiscard]] explicit constexpr operator std::size_t() const;
    [[nodiscard]] explicit constexpr operator difference_type() const noexcept;

    /**
     * @brief: The bool cast tests if this ID is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const connection_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const connection_id_t &other) const = default;

    [[nodiscard]] static constexpr auto min() noexcept -> connection_id_t;
    [[nodiscard]] static constexpr auto max() noexcept -> connection_id_t;

    constexpr auto operator++() -> connection_id_t &;
    constexpr auto operator++(int) -> connection_id_t;
};

static_assert(std::is_trivially_copyable_v<connection_id_t>);
static_assert(std::is_trivially_copy_assignable_v<connection_id_t>);
static_assert(
    explicitly_convertible_to<connection_id_t, connection_id_t::difference_type>);

//
// Implementation
//

constexpr connection_id_t::connection_id_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr connection_id_t::operator std::size_t() const {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error(
            "connection id cannot be negative when converting to std::size_t");
    }
    return static_cast<std::size_t>(value);
}

constexpr connection_id_t::operator difference_type() const noexcept {
    return difference_type {value};
}

constexpr connection_id_t::operator bool() const noexcept {
    return value >= 0;
}

inline constexpr auto connection_id_t::min() noexcept -> connection_id_t {
    return connection_id_t {0};
}

constexpr auto connection_id_t::max() noexcept -> connection_id_t {
    return connection_id_t {std::numeric_limits<value_type>::max()};
};

constexpr auto connection_id_t::operator++() -> connection_id_t & {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error("connection id cannot be negative when incrementing");
    }
    if (value >= connection_id_t::max().value) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto connection_id_t::operator++(int) -> connection_id_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

//
// Constants
//

constexpr inline auto null_connection_id = connection_id_t {-1};
static_assert(null_connection_id == connection_id_t {});

}  // namespace logicsim

#endif
