#ifndef LOGICSIM_CORE_VOCABULARY_segment_KEY_H
#define LOGICSIM_CORE_VOCABULARY_segment_KEY_H

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
 * @brief: Unique identifier to segment (wire_id & segment_index) in the layout.
 */
struct segment_key_t {
    using value_type = int64_t;
    value_type value {-1};

    using difference_type = safe_difference_t<value_type>;

    [[nodiscard]] explicit constexpr segment_key_t() = default;
    [[nodiscard]] explicit constexpr segment_key_t(integral auto value_);

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
     * @brief: The bool cast tests if this Key is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_key_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_key_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> segment_key_t;

    constexpr auto operator++() -> segment_key_t &;
    constexpr auto operator++(int) -> segment_key_t;
};

static_assert(std::is_trivially_copyable_v<segment_key_t>);
static_assert(std::is_trivially_copy_assignable_v<segment_key_t>);
static_assert(explicitly_convertible_to<segment_key_t, segment_key_t::difference_type>);

//
// Implementation
//

constexpr segment_key_t::segment_key_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr segment_key_t::operator std::size_t() const {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error(
            "segment_id cannot be negative when converting to std::size_t");
    }
    static_assert(sizeof(std::size_t) >= sizeof(value));
    return static_cast<std::size_t>(value);
}

constexpr segment_key_t::operator difference_type() const {
    static_assert(sizeof(difference_type) >= sizeof(value));
    return difference_type {value};
}

constexpr segment_key_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto segment_key_t::max() noexcept -> segment_key_t {
    return segment_key_t {std::numeric_limits<value_type>::max()};
};

constexpr auto segment_key_t::operator++() -> segment_key_t & {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error("segment_key cannot be negative when incrementing");
    }
    if (value >= max().value) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto segment_key_t::operator++(int) -> segment_key_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

//
// Constants
//

constexpr inline static auto null_segment_key = segment_key_t {-1};
static_assert(null_segment_key == segment_key_t {});

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::segment_key_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::segment_key_t &obj) const noexcept
        -> uint64_t {
        return logicsim::wyhash(obj.value);
    }
};

#endif
