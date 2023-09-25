#ifndef LOGICSIM_VOCABULARY_SEGMENT_INDEX_H
#define LOGICSIM_VOCABULARY_SEGMENT_INDEX_H

#include "format/struct.h"

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

    using difference_type = value_type;
    static_assert(std::is_signed_v<difference_type>);

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_index_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_index_t &other) const = default;

    [[nodiscard]] static constexpr auto max() noexcept {
        return std::numeric_limits<value_type>::max();
    };

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return value >= 0;
    }

    constexpr auto operator++() noexcept -> segment_index_t & {
        ++value;
        return *this;
    }

    constexpr auto operator++(int) noexcept -> segment_index_t {
        auto tmp = *this;
        operator++();
        return tmp;
    }

    constexpr auto operator--() noexcept -> segment_index_t & {
        --value;
        return *this;
    }

    constexpr auto operator--(int) noexcept -> segment_index_t {
        auto tmp = *this;
        operator--();
        return tmp;
    }
};

static_assert(std::is_trivial<segment_index_t>::value);

inline constexpr auto null_segment_index = segment_index_t {-1};

}  // namespace logicsim

#endif
