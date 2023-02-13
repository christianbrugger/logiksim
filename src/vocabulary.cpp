#include "vocabulary.h"

namespace logicsim {

auto format(ElementType type) -> std::string {
    switch (type) {
        using enum ElementType;

        case placeholder:
            return "Placeholder";
        case wire:
            return "Wire";
        case inverter_element:
            return "Inverter";
        case and_element:
            return "AndElement";
        case or_element:
            return "OrElement";
        case xor_element:
            return "XorElement";
        case clock_generator:
            return "ClockGenerator";
        case flipflop_jk:
            return "JK-FlipFlop";
        case shift_register:
            return "ShiftRegister";
    }
    throw_exception("Don't know how to convert ElementType to string.");
}

}  // namespace logicsim