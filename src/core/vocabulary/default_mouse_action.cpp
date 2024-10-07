#include "default_mouse_action.h"

#include "vocabulary/default_mouse_action.h"

#include <exception>

namespace logicsim {

template <>
auto format(DefaultMouseAction action) -> std::string {
    switch (action) {
        using enum DefaultMouseAction;

        case selection:
            return "selection";
        case insert_wire:
            return "insert_wire";

        case insert_button:
            return "insert_button";
        case insert_led:
            return "insert_led";
        case insert_text_element:
            return "insert_text";
        case insert_display_number:
            return "insert_display_number";
        case insert_display_ascii:
            return "insert_display_ascii";

        case insert_and_element:
            return "insert_and_element";
        case insert_or_element:
            return "insert_or_element";
        case insert_xor_element:
            return "insert_xor_element";
        case insert_nand_element:
            return "insert_nand_element";
        case insert_nor_element:
            return "insert_nor_element";

        case insert_buffer_element:
            return "insert_buffer_element";
        case insert_inverter_element:
            return "insert_inverter_element";
        case insert_flipflop_jk:
            return "insert_flipflop_jk";
        case insert_latch_d:
            return "insert_latch_d";
        case insert_flipflop_d:
            return "insert_flipflop_d";
        case insert_flipflop_ms_d:
            return "insert_flipflop_ms_d";

        case insert_clock_generator:
            return "insert_clock_generator";
        case insert_shift_register:
            return "insert_shift_register";
    };

    std::terminate();
}

auto is_inserting_state(DefaultMouseAction action) -> bool {
    using enum DefaultMouseAction;
    return action != selection;
}

auto is_insert_logic_item_state(DefaultMouseAction action) -> bool {
    using enum DefaultMouseAction;
    return action != selection && action != insert_wire;
}

}  // namespace logicsim
