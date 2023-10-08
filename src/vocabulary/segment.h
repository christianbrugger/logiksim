#ifndef LOGICSIM_VOCABULARY_SEGMENT_H
#define LOGICSIM_VOCABULARY_SEGMENT_H

#include "format/struct.h"
#include "vocabulary/element_id.h"
#include "vocabulary/segment_index.h"
#include "wyhash.h"

#include <ankerl/unordered_dense.h>

#include <compare>
#include <stdexcept>

namespace logicsim {

/**
 * @brief: Identifier line segment in the circuit.
 */
struct segment_t {
    element_id_t element_id {null_element};
    segment_index_t segment_index {null_segment_index};

    [[nodiscard]] explicit constexpr segment_t() noexcept = default;
    [[nodiscard]] explicit constexpr segment_t(element_id_t element_id_,
                                               segment_index_t segment_index_);
    /**
     * @brief: The bool cast tests if this segment is valid.
     */
    [[nodiscard]] explicit constexpr operator bool() const noexcept;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_t &other) const = default;
};

static_assert(std::is_trivially_copyable_v<segment_t>);
static_assert(std::is_trivially_copy_assignable_v<segment_t>);

constexpr inline auto null_segment = segment_t {};

//
// Implemention
//

constexpr segment_t::segment_t(element_id_t element_id_, segment_index_t segment_index_)
    : element_id {element_id_}, segment_index {segment_index_} {
    if ((element_id == null_element) ^ (segment_index == null_segment_index))
        [[unlikely]] {
        throw std::runtime_error("Segment cannot be partially null.");
    }
}

constexpr segment_t::operator bool() const noexcept {
    return bool {element_id};
}

}  // namespace logicsim

//
// Hash function
//

template <>
struct ankerl::unordered_dense::hash<logicsim::segment_t> {
    using is_avalanching = void;

    [[nodiscard]] auto operator()(const logicsim::segment_t &obj) const noexcept
        -> uint64_t {
        static_assert(std::is_same_v<int32_t, logicsim::element_id_t::value_type>);
        static_assert(std::is_same_v<int32_t, logicsim::segment_index_t::value_type>);

        return logicsim::wyhash_64_bit(obj.element_id.value, obj.segment_index.value);
    }
};

#endif
