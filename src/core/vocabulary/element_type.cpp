#include "core/vocabulary/element_type.h"

#include <exception>
#include <string>

namespace logicsim {

template <>
auto format(ElementType type) -> std::string {
    switch (type) {
        using enum ElementType;

            //
            // LogicItem Values
            //

        case buffer_element:
            return "Buffer";
        case and_element:
            return "AndElement";
        case or_element:
            return "OrElement";
        case xor_element:
            return "XorElement";

        case button:
            return "Button";
        case led:
            return "LED";
        case display_number:
            return "display_number";
        case display_ascii:
            return "display_ascii";

        case clock_generator:
            return "ClockGenerator";
        case flipflop_jk:
            return "JK-FlipFlop";
        case shift_register:
            return "ShiftRegister";

        case latch_d:
            return "D-Latch";
        case flipflop_d:
            return "D-FlipFlop";
        case flipflop_ms_d:
            return "MS-D-FlipFlop";

        case sub_circuit:
            return "SubCircuit";

            //
            // Additional Values
            //

        case unused:
            return "Unused";
        case placeholder:
            return "Placeholder";
        case wire:
            return "Wire";
    }
    std::terminate();
}

auto is_logicitem(ElementType element_type) -> bool {
    using enum ElementType;
    return element_type != unused && element_type != placeholder && element_type != wire;
}

}  // namespace logicsim
