#ifndef LOGICSIM_VOCABULARY_INSERTION_MODE_H
#define LOGICSIM_VOCABULARY_INSERTION_MODE_H

#include "core/format/enum.h"
#include "core/vocabulary/display_state.h"

#include <cstdint>

namespace logicsim {

/**
 * @brief: Specify how an element shall be added to the circuit.
 */
enum class InsertionMode {
    insert_or_discard,
    collisions,
    temporary,
};

template <>
[[nodiscard]] auto format(InsertionMode mode) -> std::string;

[[nodiscard]] auto to_insertion_mode(display_state_t display_state) -> InsertionMode;

/**
 * @brief: Specify additional hints for inserting wires.
 */
enum class SegmentInsertionHint {
    no_hint,
    /**
     * brief: Wires are assumed to be colliding. Collision check is skipped.
     */
    assume_colliding,
};

template <>
[[nodiscard]] auto format(SegmentInsertionHint hint) -> std::string;

[[nodiscard]] auto segment_insertion_hint_valid(InsertionMode mode,
                                                SegmentInsertionHint hint) -> bool;

}  // namespace logicsim

#endif
