#ifndef LOGICSIM_VOCABULARY_INTERNAL_STATE_H
#define LOGICSIM_VOCABULARY_INTERNAL_STATE_H

#include "format/struct.h"
#include "vocabulary/element_id.h"
#include "vocabulary/internal_state_index.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>

#include <compare>

namespace logicsim {

struct internal_state_t {
    element_id_t element_id;
    internal_state_index_t internal_state_index;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const internal_state_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const internal_state_t &other) const = default;
};

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::internal_state_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::internal_state_t &obj) const noexcept
        -> uint64_t {
        static_assert(std::is_same_v<int32_t, logicsim::element_id_t::value_type>);
        static_assert(
            std::is_same_v<uint32_t, logicsim::internal_state_index_t::value_type>);

        return logicsim::wyhash_64_bit(obj.element_id.value,
                                       obj.internal_state_index.value);
    }
};

#endif
