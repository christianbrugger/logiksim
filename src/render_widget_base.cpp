
#include "render_widget_base.h"

#include "layout_calculation.h"
#include "vocabulary/element_definition.h"

namespace logicsim {

auto to_logic_item_definition(InteractionState state) -> ElementDefinition {
    switch (state) {
        using enum InteractionState;

        case not_interactive:
        case selection:
        case simulation:
            throw_exception("non-inserting states don't have a definition");

        case insert_wire:
            return ElementDefinition {
                .element_type = ElementType::wire,
                .input_count = connection_count_t {0},
                .output_count = connection_count_t {0},
                .orientation = orientation_t::undirected,
            };
        case insert_button:
            return ElementDefinition {
                .element_type = ElementType::button,
                .input_count = connection_count_t {0},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::undirected,
            };
        case insert_led:
            return ElementDefinition {
                .element_type = ElementType::led,
                .input_count = connection_count_t {1},
                .output_count = connection_count_t {0},
                .orientation = orientation_t::undirected,
            };
        case insert_display_number:
            return ElementDefinition {
                .element_type = ElementType::display_number,
                .input_count = connection_count_t {3} + display_number::control_inputs,
                .output_count = connection_count_t {0},
                .orientation = orientation_t::right,
            };
        case insert_display_ascii:
            return ElementDefinition {
                .element_type = ElementType::display_ascii,
                .input_count = display_ascii::input_count,
                .output_count = connection_count_t {0},
                .orientation = orientation_t::right,
            };

        case insert_and_element:
            return ElementDefinition {
                .element_type = ElementType::and_element,
                .input_count = connection_count_t {2},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
            };
        case insert_or_element:
            return ElementDefinition {
                .element_type = ElementType::or_element,
                .input_count = connection_count_t {2},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
            };
        case insert_xor_element:
            return ElementDefinition {
                .element_type = ElementType::xor_element,
                .input_count = connection_count_t {2},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
            };
        case insert_nand_element:
            return ElementDefinition {
                .element_type = ElementType::and_element,
                .input_count = connection_count_t {2},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
                .output_inverters = logic_small_vector_t {true},
            };
        case insert_nor_element:
            return ElementDefinition {
                .element_type = ElementType::or_element,
                .input_count = connection_count_t {2},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
                .output_inverters = logic_small_vector_t {true},
            };

        case insert_buffer_element:
            return ElementDefinition {
                .element_type = ElementType::buffer_element,
                .input_count = connection_count_t {1},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
            };
        case insert_inverter_element:
            return ElementDefinition {
                .element_type = ElementType::buffer_element,
                .input_count = connection_count_t {1},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
                .output_inverters = logic_small_vector_t {true},
            };

        case insert_flipflop_jk:
            return ElementDefinition {
                .element_type = ElementType::flipflop_jk,
                .input_count = connection_count_t {5},
                .output_count = connection_count_t {2},
                .orientation = orientation_t::right,
            };
        case insert_latch_d:
            return ElementDefinition {
                .element_type = ElementType::latch_d,
                .input_count = connection_count_t {2},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
            };
        case insert_flipflop_d:
            return ElementDefinition {
                .element_type = ElementType::flipflop_d,
                .input_count = connection_count_t {4},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
            };
        case insert_flipflop_ms_d:
            return ElementDefinition {
                .element_type = ElementType::flipflop_ms_d,
                .input_count = connection_count_t {4},
                .output_count = connection_count_t {1},
                .orientation = orientation_t::right,
            };

        case insert_clock_generator:
            return ElementDefinition {
                .element_type = ElementType::clock_generator,
                .input_count = connection_count_t {3},
                .output_count = connection_count_t {3},
                .orientation = orientation_t::right,

                .attrs_clock_generator = attributes_clock_generator_t {},
            };
        case insert_shift_register:
            return ElementDefinition {
                .element_type = ElementType::shift_register,
                .input_count = connection_count_t {3},
                .output_count = connection_count_t {2},
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