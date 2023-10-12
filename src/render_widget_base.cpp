
#include "render_widget_base.h"

#include "exception.h"
#include "layout_info.h"
#include "vocabulary/element_definition.h"

#include <exception>
#include <stdexcept>

namespace logicsim {

namespace {
auto default_element_definition(ElementType element_type) -> ElementDefinition {
    return ElementDefinition {
        .element_type = element_type,
        .input_count = element_input_count_default(element_type),
        .output_count = element_output_count_default(element_type),
        .orientation = element_direction_type(element_type) == DirectionType::directed
                           ? orientation_t::right
                           : orientation_t::undirected,
    };
}
}  // namespace

auto to_logic_item_definition(InteractionState state) -> ElementDefinition {
    switch (state) {
        using enum InteractionState;

        case not_interactive:
        case selection:
        case simulation:
            throw std::runtime_error("non-inserting states don't have a definition");

        case insert_wire:
            return default_element_definition(ElementType::wire);
        case insert_button:
            return default_element_definition(ElementType::button);
        case insert_led:
            return default_element_definition(ElementType::led);
        case insert_display_number:
            return default_element_definition(ElementType::display_number);
        case insert_display_ascii:
            return default_element_definition(ElementType::display_ascii);

        case insert_and_element:
            return default_element_definition(ElementType::and_element);
        case insert_or_element:
            return default_element_definition(ElementType::or_element);
        case insert_xor_element:
            return default_element_definition(ElementType::xor_element);

        case insert_nand_element: {
            auto definition = default_element_definition(ElementType::xor_element);
            definition.output_inverters = logic_small_vector_t {true};
            return definition;
        }
        case insert_nor_element: {
            auto definition = default_element_definition(ElementType::or_element);
            definition.output_inverters = logic_small_vector_t {true};
            return definition;
        }

        case insert_buffer_element:
            return default_element_definition(ElementType::buffer_element);
        case insert_inverter_element: {
            auto definition = default_element_definition(ElementType::buffer_element);
            definition.output_inverters = logic_small_vector_t {true};
            return definition;
        }

        case insert_flipflop_jk:
            return default_element_definition(ElementType::flipflop_jk);
        case insert_latch_d:
            return default_element_definition(ElementType::latch_d);
        case insert_flipflop_d:
            return default_element_definition(ElementType::flipflop_d);
        case insert_flipflop_ms_d:
            return default_element_definition(ElementType::flipflop_ms_d);

        case insert_clock_generator: {
            auto definition = default_element_definition(ElementType::clock_generator);
            definition.attrs_clock_generator = attributes_clock_generator_t {};
            return definition;
        }
        case insert_shift_register:
            return default_element_definition(ElementType::shift_register);
    }
    std::terminate();
}

auto RendererWidgetBase::emit_interaction_state_changed(InteractionState new_state)
    -> void {
    emit interaction_state_changed(new_state);
}

}  // namespace logicsim