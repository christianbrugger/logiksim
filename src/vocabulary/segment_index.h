#ifndef LOGICSIM_VOCABULARY_SEGMENT_INDEX_H
#define LOGICSIM_VOCABULARY_SEGMENT_INDEX_H

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
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    /**
     * @brief: The bool cast tests if this index is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;
    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_index_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_index_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept -> segment_index_t;

    constexpr auto operator++() noexcept -> segment_index_t &;
    constexpr auto operator++(int) noexcept -> segment_index_t;
    constexpr auto operator--() noexcept -> segment_index_t &;
    constexpr auto operator--(int) noexcept -> segment_index_t;
};

static_assert(std::is_aggregate_v<segment_index_t>);
static_assert(std::is_trivial_v<segment_index_t>);

inline constexpr auto null_segment_index = segment_index_t {-1};

//
// Implementation
//

constexpr segment_index_t::operator bool() const noexcept {
    return value >= 0;
}

constexpr auto segment_index_t::max() noexcept -> segment_index_t {
    return segment_index_t {std::numeric_limits<value_type>::max()};
};

constexpr auto segment_index_t::operator++() noexcept -> segment_index_t & {
    ++value;
    return *this;
}

constexpr auto segment_index_t::operator++(int) noexcept -> segment_index_t {
    auto tmp = *this;
    operator++();
    return tmp;
}

constexpr auto segment_index_t::operator--() noexcept -> segment_index_t & {
    --value;
    return *this;
}

constexpr auto segment_index_t::operator--(int) noexcept -> segment_index_t {
    auto tmp = *this;
    operator--();
    return tmp;
}

}  // namespace logicsim

#endif
