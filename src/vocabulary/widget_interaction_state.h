#ifndef LOGICSIM_VOCABULARY_WIDGET_INTERACTION_STATE_H
#define LOGICSIM_VOCABULARY_WIDGET_INTERACTION_STATE_H

#include "format/enum.h"

#include <string>

namespace logicsim {

enum class InteractionState {
    not_interactive,
    selection,
    simulation,

    insert_wire,
    insert_button,
    insert_led,
    insert_display_number,
    insert_display_ascii,

    insert_and_element,
    insert_or_element,
    insert_xor_element,
    insert_nand_element,
    insert_nor_element,

    insert_buffer_element,
    insert_inverter_element,
    insert_flipflop_jk,
    insert_latch_d,
    insert_flipflop_d,
    insert_flipflop_ms_d,

    insert_clock_generator,
    insert_shift_register,
};

template <>
auto format(InteractionState type) -> std::string;

[[nodiscard]] auto is_inserting_state(InteractionState state) -> bool;

}  // namespace logicsim

#endif
