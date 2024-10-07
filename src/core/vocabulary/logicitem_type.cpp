#include "vocabulary/logicitem_type.h"

#include <exception>
#include <string>

namespace logicsim {

template <>
auto format(LogicItemType type) -> std::string {
    switch (type) {
        using enum LogicItemType;

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
        case text_element:
            return "TextElement";
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
    }
    std::terminate();
}

}  // namespace logicsim
