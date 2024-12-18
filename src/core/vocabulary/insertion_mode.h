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
enum class InsertionHint {
    no_hint,
    /**
     * brief: Assume element is colliding during collision check when inserting
     */
    assume_colliding,
};

template <>
[[nodiscard]] auto format(InsertionHint hint) -> std::string;

[[nodiscard]] auto insertion_hint_valid(InsertionMode mode, InsertionHint hint) -> bool;

}  // namespace logicsim

#endif
