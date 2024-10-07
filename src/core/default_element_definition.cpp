#include "default_element_definition.h"

#include "layout_info.h"
#include "vocabulary/decoration_definition.h"
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
        .attrs_clock_generator = logicitem_type == LogicItemType::clock_generator
                                     ? std::make_optional(attributes_clock_generator_t {})
                                     : std::nullopt,
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
            return default_element_definition(LogicItemType::clock_generator);
        }
        case insert_shift_register:
            return default_element_definition(LogicItemType::shift_register);

            //
            // Decorations
            //

        case insert_decoration_text_element:
            throw std::runtime_error("decorations don't have a logic item definition");
    }
    std::terminate();
}

//
// Decorations
//

auto default_decoration_definition(DecorationType decoration_type)
    -> DecorationDefinition {
    switch (decoration_type) {
        using enum DecorationType;

        case text_element:
            return DecorationDefinition {
                .decoration_type = DecorationType::text_element,
                .attrs_text_element = attributes_text_element_t {.text = "new text"},
            };
    }
    std::terminate();
}

auto to_decoration_definition(DefaultMouseAction mouse_action) -> DecorationDefinition {
    switch (mouse_action) {
        using enum DefaultMouseAction;

        case selection:
            throw std::runtime_error("non-inserting states don't have a definition");
        case insert_wire:
            throw std::runtime_error("wires don't have a decoration definition");

        case insert_button:
        case insert_led:
        case insert_display_number:
        case insert_display_ascii:

        case insert_and_element:
        case insert_or_element:
        case insert_xor_element:

        case insert_nand_element:
        case insert_nor_element:

        case insert_buffer_element:
        case insert_inverter_element:

        case insert_flipflop_jk:
        case insert_latch_d:
        case insert_flipflop_d:
        case insert_flipflop_ms_d:

        case insert_clock_generator:
        case insert_shift_register:
            throw std::runtime_error("logic items don't have a decoration definition");

            //
            // Decorations
            //

        case insert_decoration_text_element:
            return default_decoration_definition(DecorationType::text_element);
    }
    std::terminate();
}

}  // namespace logicsim
