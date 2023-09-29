#ifndef LOGICSIM_VOCABULARY_SEGMENT_PART_H
#define LOGICSIM_VOCABULARY_SEGMENT_PART_H

#include "format/struct.h"
#include "vocabulary/part.h"
#include "vocabulary/segment.h"

#include <compare>
#include <type_traits>

namespace logicsim {

/**
 * @brief: Identifier a sub-segment a specific wire segment in the circuit.
 *
 * This is the smallest granularity of operations or selections on wires.
 */
struct segment_part_t {
    segment_t segment;
    part_t part;

    [[nodiscard]] auto format() const -> std::string;

    [[nodiscard]] auto operator==(const segment_part_t &other) const -> bool = default;
    [[nodiscard]] auto operator<=>(const segment_part_t &other) const = default;

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return bool {segment};
    }
};

static_assert(std::is_aggregate_v<segment_part_t>);

constexpr inline auto null_segment_part =
    segment_part_t {null_segment, part_t {offset_t {0}, offset_t {1}}};

}  // namespace logicsim

#endif
