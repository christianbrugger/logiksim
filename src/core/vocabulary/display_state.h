#ifndef LOGICSIM_VOCABULARY_DISPLAY_STATE_H
#define LOGICSIM_VOCABULARY_DISPLAY_STATE_H

#include "core/format/enum.h"

#include <array>
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
[[nodiscard]] auto format(display_state_t state) -> std::string;

[[nodiscard]] auto is_inserted(display_state_t display_state) -> bool;

constexpr inline auto all_display_states = std::array {
    display_state_t::normal,     //
    display_state_t::valid,      //
    display_state_t::colliding,  //
    display_state_t::temporary,  //
};

}  // namespace logicsim

#endif
