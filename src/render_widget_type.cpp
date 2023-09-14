
#include "render_widget_type.h"

#include "editable_circuit/type.h"
#include "layout_calculation.h"

namespace logicsim {

template <>
auto format(InteractionState state) -> std::string {
    switch (state) {
        using enum InteractionState;

        case not_interactive:
            return "not_interactive";
        case selection:
            return "selection";
        case simulation:
            return "simulation";

        case insert_wire:
            return "insert_wire";
        case insert_button:
            return "insert_button";
        case insert_led:
            return "insert_led";
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
    }
    throw_exception("Don't know how to convert InteractionState to string.");
}

auto is_inserting_state(InteractionState state) -> bool {
    using enum InteractionState;
    return state != not_interactive && state != selection && state != simulation;
}

auto to_logic_item_definition(InteractionState state, std::size_t variable_input_count)
    -> LogicItemDefinition {
    switch (state) {
        using enum InteractionState;

        case not_interactive:
        case selection:
        case simulation:
            throw_exception("non-inserting states don't have a definition");

        case insert_wire:
            return LogicItemDefinition {
                .element_type = ElementType::wire,
                .input_count = 0,
                .output_count = 0,
                .orientation = orientation_t::undirected,
            };
        case insert_button:
            return LogicItemDefinition {
                .element_type = ElementType::button,
                .input_count = 0,
                .output_count = 1,
                .orientation = orientation_t::undirected,
            };
        case insert_led:
            return LogicItemDefinition {
                .element_type = ElementType::led,
                .input_count = 1,
                .output_count = 0,
                .orientation = orientation_t::undirected,
            };
        case insert_display_number:
            return LogicItemDefinition {
                .element_type = ElementType::display_number,
                .input_count =
                    std::clamp(variable_input_count + display_number::control_inputs,
                               display_number::min_inputs, display_number::max_inputs),
                .output_count = 0,
                .orientation = orientation_t::right,
            };
        case insert_display_ascii:
            return LogicItemDefinition {
                .element_type = ElementType::display_ascii,
                .input_count = display_ascii::input_count,
                .output_count = 0,
                .orientation = orientation_t::right,
            };

        case insert_and_element:
            return LogicItemDefinition {
                .element_type = ElementType::and_element,
                .input_count =
                    std::clamp(variable_input_count, standard_element::min_inputs,
                               standard_element::max_inputs),
                .output_count = 1,
                .orientation = orientation_t::right,
            };
        case insert_or_element:
            return LogicItemDefinition {
                .element_type = ElementType::or_element,
                .input_count =
                    std::clamp(variable_input_count, standard_element::min_inputs,
                               standard_element::max_inputs),
                .output_count = 1,
                .orientation = orientation_t::right,
            };
        case insert_xor_element:
            return LogicItemDefinition {
                .element_type = ElementType::xor_element,
                .input_count =
                    std::clamp(variable_input_count, standard_element::min_inputs,
                               standard_element::max_inputs),
                .output_count = 1,
                .orientation = orientation_t::right,
            };
        case insert_nand_element:
            return LogicItemDefinition {
                .element_type = ElementType::and_element,
                .input_count =
                    std::clamp(variable_input_count, standard_element::min_inputs,
                               standard_element::max_inputs),
                .output_count = 1,
                .orientation = orientation_t::right,
                .output_inverters = logic_small_vector_t {true},
            };
        case insert_nor_element:
            return LogicItemDefinition {
                .element_type = ElementType::or_element,
                .input_count =
                    std::clamp(variable_input_count, standard_element::min_inputs,
                               standard_element::max_inputs),
                .output_count = 1,
                .orientation = orientation_t::right,
                .output_inverters = logic_small_vector_t {true},
            };

        case insert_buffer_element:
            return LogicItemDefinition {
                .element_type = ElementType::buffer_element,
                .input_count = 1,
                .output_count = 1,
                .orientation = orientation_t::right,
            };
        case insert_inverter_element:
            return LogicItemDefinition {
                .element_type = ElementType::buffer_element,
                .input_count = 1,
                .output_count = 1,
                .orientation = orientation_t::right,
                .output_inverters = logic_small_vector_t {true},
            };

        case insert_flipflop_jk:
            return LogicItemDefinition {
                .element_type = ElementType::flipflop_jk,
                .input_count = 5,
                .output_count = 2,
                .orientation = orientation_t::right,
            };
        case insert_latch_d:
            return LogicItemDefinition {
                .element_type = ElementType::latch_d,
                .input_count = 2,
                .output_count = 1,
                .orientation = orientation_t::right,
            };
        case insert_flipflop_d:
            return LogicItemDefinition {
                .element_type = ElementType::flipflop_d,
                .input_count = 4,
                .output_count = 1,
                .orientation = orientation_t::right,
            };
        case insert_flipflop_ms_d:
            return LogicItemDefinition {
                .element_type = ElementType::flipflop_ms_d,
                .input_count = 4,
                .output_count = 1,
                .orientation = orientation_t::right,
            };

        case insert_clock_generator:
            return LogicItemDefinition {
                .element_type = ElementType::clock_generator,
                .input_count = 2,
                .output_count = 2,
                .orientation = orientation_t::right,

                .attrs_clock_generator = layout::attributes_clock_generator {},
            };
        case insert_shift_register:
            return LogicItemDefinition {
                .element_type = ElementType::shift_register,
                .input_count = 3,
                .output_count = 2,
                .orientation = orientation_t::right,
            };
    }
    throw_exception("Don't know how to convert InteractionState to definition.");
}

auto RendererWidgetBase::emit_interaction_state_changed(InteractionState new_state)
    -> void {
    emit interaction_state_changed(new_state);
}

}  // namespace logicsim