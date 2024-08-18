#ifndef LOGICSIM_VOCABULARY_INSERTION_MODE_H
#define LOGICSIM_VOCABULARY_INSERTION_MODE_H

#include "format/enum.h"
#include "vocabulary/display_state.h"

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

}  // namespace logicsim

#endif
