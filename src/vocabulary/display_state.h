#ifndef LOGICSIM_VOCABULARY_DISPLAY_STATE_H
#define LOGICSIM_VOCABULARY_DISPLAY_STATE_H

#include "format/enum.h"

#include <cstdint>

namespace logicsim {

/**
 * @brief: The state of an element in the circuit.
 */
enum class display_state_t : uint8_t {
    normal,

    valid,
    colliding,

    temporary,
};

template <>
auto format(display_state_t state) -> std::string;

[[nodiscard]] auto is_inserted(display_state_t display_state) -> bool;

}

#endif
