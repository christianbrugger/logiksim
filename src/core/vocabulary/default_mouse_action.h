#ifndef LOGICSIM_VOCABULARY_DEFAULT_MOUSE_ACTION_H
#define LOGICSIM_VOCABULARY_DEFAULT_MOUSE_ACTION_H

#include "format/enum.h"

namespace logicsim {

enum class DefaultMouseAction {
    // other
    selection,
    insert_wire,

    // logic items
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

    // decorations
    insert_decoration_text_element,
};

template <>
[[nodiscard]] auto format(DefaultMouseAction action) -> std::string;

[[nodiscard]] auto is_inserting_state(DefaultMouseAction action) -> bool;
[[nodiscard]] auto is_insert_logicitem_state(DefaultMouseAction action) -> bool;
[[nodiscard]] auto is_insert_decoration_state(DefaultMouseAction action) -> bool;

}  // namespace logicsim

#endif
