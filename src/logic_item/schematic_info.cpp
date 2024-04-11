#include "logic_item/schematic_info.h"

#include "logic_item/layout.h"
#include "vocabulary/internal_connection.h"

#include <exception>

namespace logicsim {

namespace {

constexpr static inline auto logic_item_delay = delay_t {3us};

constexpr static inline auto button_delay = delay_t::epsilon();
constexpr static inline auto clock_generator_output_delay = delay_t::epsilon();

}  // namespace

auto to_element_type(LogicItemType logicitem_type) -> ElementType {
    switch (logicitem_type) {
        using enum LogicItemType;

        case buffer_element:
            return ElementType::buffer_element;
        case and_element:
            return ElementType::and_element;
        case or_element:
            return ElementType::or_element;
        case xor_element:
            return ElementType::xor_element;

        case button:
            return ElementType::button;
        case led:
            return ElementType::led;
        case display_number:
            return ElementType::display_number;
        case display_ascii:
            return ElementType::display_ascii;

        case clock_generator:
            return ElementType::clock_generator;
        case flipflop_jk:
            return ElementType::flipflop_jk;
        case shift_register:
            return ElementType::shift_register;

        case latch_d:
            return ElementType::latch_d;
        case flipflop_d:
            return ElementType::flipflop_d;
        case flipflop_ms_d:
            return ElementType::flipflop_ms_d;

        case sub_circuit:
            return ElementType::sub_circuit;
    }
    std::terminate();
}

auto to_logicitem_type(ElementType logicitem_type) -> LogicItemType {
    switch (logicitem_type) {
        using enum ElementType;

        case buffer_element:
            return LogicItemType::buffer_element;
        case and_element:
            return LogicItemType::and_element;
        case or_element:
            return LogicItemType::or_element;
        case xor_element:
            return LogicItemType::xor_element;

        case button:
            return LogicItemType::button;
        case led:
            return LogicItemType::led;
        case display_number:
            return LogicItemType::display_number;
        case display_ascii:
            return LogicItemType::display_ascii;

        case clock_generator:
            return LogicItemType::clock_generator;
        case flipflop_jk:
            return LogicItemType::flipflop_jk;
        case shift_register:
            return LogicItemType::shift_register;

        case latch_d:
            return LogicItemType::latch_d;
        case flipflop_d:
            return LogicItemType::flipflop_d;
        case flipflop_ms_d:
            return LogicItemType::flipflop_ms_d;

        case sub_circuit:
            return LogicItemType::sub_circuit;

            //
            // Schematic Only types
            //

        case unused:
        case placeholder:
        case wire:
            throw std::runtime_error("element-type is not convertible to logicitem-type");
    }
    std::terminate();
}

auto element_enable_input_id(ElementType element_type) -> std::optional<connection_id_t> {
    if (is_logic_item(element_type)) {
        const auto logicitem_type = to_logicitem_type(element_type);
        return get_layout_info(logicitem_type).enable_input_id;
    }
    return std::nullopt;
}

auto element_output_delay(LogicItemType logicitem_type) -> delay_t {
    switch (logicitem_type) {
        using enum LogicItemType;

        case button:
            return button_delay;
        case clock_generator:
            return clock_generator_output_delay;

        default:
            return logic_item_delay;
    };
}

auto element_internal_connections(ElementType element_type) -> internal_connections_t {
    if (element_type == ElementType::clock_generator) {
        return {
            internal_connection_t {
                .output = connection_id_t {1},
                .input = connection_id_t {1},
            },
            internal_connection_t {
                .output = connection_id_t {2},
                .input = connection_id_t {2},
            },
        };
    }

    return {};
}

auto has_internal_connections(ElementType element_type) -> bool {
    return element_internal_connections(element_type).size() != 0;
}

auto is_input_output_count_valid(ElementType element_type, connection_count_t input_count,
                                 connection_count_t output_count) -> bool {
    if (is_logic_item(element_type)) {
        const auto logicitem_type = to_logicitem_type(element_type);
        return layout_info::is_input_output_count_valid(logicitem_type, input_count,
                                                        output_count);
    }

    if (element_type == ElementType::unused) {
        return input_count == connection_count_t {0} &&
               output_count == connection_count_t {0};
    }

    if (element_type == ElementType::placeholder) {
        return input_count == connection_count_t {1} &&
               output_count == connection_count_t {0};
    }

    if (element_type == ElementType::wire) {
        return input_count <= connection_count_t {1};
    }

    std::terminate();
}

}  // namespace logicsim
