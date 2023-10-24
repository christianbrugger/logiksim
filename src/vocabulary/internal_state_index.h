#ifndef LOGICSIM_VOCABULARY_INTERNAL_STATE_INDEX_H
#define LOGICSIM_VOCABULARY_INTERNAL_STATE_INDEX_H

#include "format/struct.h"
#include "type_trait/safe_difference_type.h"

#include <compare>
#include <cstdint>

namespace logicsim {

/**
 * @brief: Identifier to a internal state.
 */
struct internal_state_index_t {
    using value_type = uint32_t;
    value_type value;

    using difference_type = safe_difference_t<value_type>;
    static_assert(sizeof(difference_type) > sizeof(value_type));

    [[nodiscard]] explicit constexpr operator std::size_t() const;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const internal_state_index_t &other) const
        -> bool = default;
    [[nodiscard]] auto operator<=>(const internal_state_index_t &other) const = default;
};

//
// Implementation
//

constexpr internal_state_index_t::operator std::size_t() const {
    return std::size_t {value};
}

}  // namespace logicsim

#endif
