#ifndef LOGICSIM_VOCABULARY_SEGMENT_INDEX_H
#define LOGICSIM_VOCABULARY_SEGMENT_INDEX_H

#include "algorithm/narrow_integral.h"
#include "concept/explicitly_convertible.h"
#include "concept/integral.h"
#include "format/struct.h"
#include "type_trait/safe_difference_type.h"

#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifier to a segment in a segment tree.
 */
struct segment_index_t {
    using value_type = int32_t;
    value_type value {-1};

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr segment_index_t() = default;
    [[nodiscard]] explicit constexpr segment_index_t(integral auto value_);

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
     * @brief: The bool cast tests if this index is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_index_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_index_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> segment_index_t;

    constexpr auto operator++() -> segment_index_t &;
    constexpr auto operator++(int) -> segment_index_t;

    constexpr auto operator--() -> segment_index_t &;
    constexpr auto operator--(int) -> segment_index_t;
};

static_assert(std::is_trivially_copyable_v<segment_index_t>);
static_assert(std::is_trivially_copy_assignable_v<segment_index_t>);
static_assert(
    explicitly_convertible_to<segment_index_t, segment_index_t::difference_type>);

//
// Implementation
//

constexpr segment_index_t::segment_index_t(integral auto value_)
    : value {narrow_integral<value_type>(value_)} {}

constexpr segment_index_t::operator std::size_t() const {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error(
            "segment index cannot be negative when converting to std::size_t");
    }
    return static_cast<std::size_t>(value);
}

constexpr segment_index_t::operator difference_type() const {
    return difference_type {value};
}

constexpr segment_index_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto segment_index_t::max() noexcept -> segment_index_t {
    return segment_index_t {std::numeric_limits<value_type>::max()};
};

constexpr auto segment_index_t::operator++() -> segment_index_t & {
    if (value < value_type {0}) [[unlikely]] {
        throw std::runtime_error("element id cannot be negative when incrementing");
    }
    if (value >= max().value) [[unlikely]] {
        throw std::overflow_error("cannot increment, overflow");
    }
    ++value;
    return *this;
}

constexpr auto segment_index_t::operator++(int) -> segment_index_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

constexpr auto segment_index_t::operator--() -> segment_index_t & {
    if (value <= value_type {0}) [[unlikely]] {
        throw std::runtime_error("cannot decrement, overflow");
    }
    --value;
    return *this;
}

constexpr auto segment_index_t::operator--(int) -> segment_index_t {
    auto tmp = *this;
    operator--();
    return tmp;
}

//
// Constants
//

inline constexpr auto null_segment_index = segment_index_t {-1};
static_assert(null_segment_index == segment_index_t {});

}  // namespace logicsim

#endif
