#include "default_element_definition.h"

#include "layout_info.h"
#include "vocabulary/logicitem_definition.h"

namespace logicsim {

auto default_element_definition(LogicItemType logicitem_type) -> LogicItemDefinition {
    return LogicItemDefinition {
        .logicitem_type = logicitem_type,
        .input_count = element_input_count_default(logicitem_type),
        .output_count = element_output_count_default(logicitem_type),
        .orientation = element_direction_type(logicitem_type) == DirectionType::directed
                           ? orientation_t::right
                           : orientation_t::undirected,
    };
}

auto to_logic_item_definition(DefaultMouseAction mouse_action) -> LogicItemDefinition {
    switch (mouse_action) {
        using enum DefaultMouseAction;

        case selection:
            throw std::runtime_error("non-inserting states don't have a definition");
        case insert_wire:
            throw std::runtime_error("wires don't have a logic item definition");

        case insert_button:
            return default_element_definition(LogicItemType::button);
        case insert_led:
            return default_element_definition(LogicItemType::led);
        case insert_text_element:
            return default_element_definition(LogicItemType::text_element);
        case insert_display_number:
            return default_element_definition(LogicItemType::display_number);
        case insert_display_ascii:
            return default_element_definition(LogicItemType::display_ascii);

        case insert_and_element:
            return default_element_definition(LogicItemType::and_element);
        case insert_or_element:
            return default_element_definition(LogicItemType::or_element);
        case insert_xor_element:
            return default_element_definition(LogicItemType::xor_element);

        case insert_nand_element: {
            auto definition = default_element_definition(LogicItemType::and_element);
            definition.output_inverters = logic_small_vector_t {true};
            return definition;
        }
        case insert_nor_element: {
            auto definition = default_element_definition(LogicItemType::or_element);
            definition.output_inverters = logic_small_vector_t {true};
            return definition;
        }

        case insert_buffer_element:
            return default_element_definition(LogicItemType::buffer_element);
        case insert_inverter_element: {
            auto definition = default_element_definition(LogicItemType::buffer_element);
            definition.output_inverters = logic_small_vector_t {true};
            return definition;
        }

        case insert_flipflop_jk:
            return default_element_definition(LogicItemType::flipflop_jk);
        case insert_latch_d:
            return default_element_definition(LogicItemType::latch_d);
        case insert_flipflop_d:
            return default_element_definition(LogicItemType::flipflop_d);
        case insert_flipflop_ms_d:
            return default_element_definition(LogicItemType::flipflop_ms_d);

        case insert_clock_generator: {
            auto definition = default_element_definition(LogicItemType::clock_generator);
            definition.attrs_clock_generator = attributes_clock_generator_t {};
            return definition;
        }
        case insert_shift_register:
            return default_element_definition(LogicItemType::shift_register);
    }
    std::terminate();
}
}  // namespace logicsim
