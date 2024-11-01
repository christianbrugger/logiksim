#ifndef LOGICSIM_CORE_VOCABULARY_DECORATION_ID_H
#define LOGICSIM_CORE_VOCABULARY_DECORATION_ID_H

#include "core/algorithm/narrow_integral.h"
#include "core/concept/explicitly_convertible.h"
#include "core/concept/integral.h"
#include "core/format/struct.h"
#include "core/type_trait/safe_difference_type.h"
#include "core/wyhash.h"

#include <ankerl/unordered_dense.h>
#include <gsl/gsl>

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifier to a decoration in the layout.
 */
struct decoration_id_t {
    using value_type = int32_t;
    value_type value {-1};

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr decoration_id_t() = default;
    [[nodiscard]] explicit constexpr decoration_id_t(integral auto value_);

    /**
     * @brief: The conversion to std::size_t
     *
     * Note When indexing arrays use .at(id.value) instead, due to performance reasons.
     *
     * Throws exception for negative / invalid ids.
     */
    [[nodiscard]] explicit constexpr operator std::size_t() const;

    [[nodiscard]] explicit constexpr operator difference_type() const;

    /**
     * @brief: The bool cast tests if this ID is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const decoration_id_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const decoration_id_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> decoration_id_t;

    constexpr auto operator++() -> decoration_id_t &;
    constexpr auto operator++(int) -> decoration_id_t;
};

static_assert(std::is_trivially_copyable_v<decoration_id_t>);
static_assert(std::is_trivially_copy_assignable_v<decoration_id_t>);
static_assert(
    explicitly_convertible_to<decoration_id_t, decoration_id_t::difference_type>);

//
// Implementation
//

constexpr decoration_id_t::decoration_id_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr decoration_id_t::operator std::size_t() const {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error(
            "decoration_id cannot be negative when converting to std::size_t");
    }
    return static_cast<std::size_t>(value);
}

constexpr decoration_id_t::operator difference_type() const {
    return difference_type {value};
}

constexpr decoration_id_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto decoration_id_t::max() noexcept -> decoration_id_t {
    return decoration_id_t {std::numeric_limits<value_type>::max()};
};

constexpr auto decoration_id_t::operator++() -> decoration_id_t & {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error("decoration_id cannot be negative when incrementing");
    }
    if (value >= max().value) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto decoration_id_t::operator++(int) -> decoration_id_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

//
// Constants
//

constexpr inline static auto null_decoration_id = decoration_id_t {-1};
static_assert(null_decoration_id == decoration_id_t {});

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::decoration_id_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::decoration_id_t &obj) const noexcept
        -> uint64_t {
        return logicsim::wyhash(obj.value);
    }
};

#endif
