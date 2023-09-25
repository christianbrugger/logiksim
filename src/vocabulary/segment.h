#ifndef LOGICSIM_VOCABULARY_SEGMENT_H
#define LOGICSIM_VOCABULARY_SEGMENT_H

#include "format/struct.h"
#include "vocabulary/element_id.h"
#include "vocabulary/segment_index.h"

#include <compare>
#include <stdexcept>

namespace logicsim {

/**
 * @brief: Identifier line segment in the circuit.
 */
struct segment_t {
    element_id_t element_id {null_element};
    segment_index_t segment_index {null_segment_index};

    constexpr segment_t() noexcept = default;

    constexpr segment_t(element_id_t element_id_, segment_index_t segment_index_)
        : element_id {element_id_}, segment_index {segment_index_} {
        if ((element_id == null_element) ^ (segment_index == null_segment_index))
            [[unlikely]] {
            throw std::runtime_error("Segment cannot be partially null.");
        }
    }

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_t &other) const = default;

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return element_id.value >= 0;
    }
};

constexpr inline auto null_segment = segment_t {};

}  // namespace logicsim

#endif
