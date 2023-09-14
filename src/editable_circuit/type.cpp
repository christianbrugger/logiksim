#include "editable_circuit/type.h"

#include "exception.h"
#include "layout_calculation.h"

#include <fmt/core.h>

namespace logicsim {

template <>
auto format(LineInsertionType type) -> std::string {
    switch (type) {
        using enum LineInsertionType;

        case horizontal_first:
            return "horizontal_first";
        case vertical_first:
            return "vertical_first";
    }
    throw_exception("unknown LineInsertionType");
}

auto LogicItemDefinition::is_valid() const -> bool {
    using enum ElementType;

    return is_input_output_count_valid(element_type, input_count, output_count) &&
           is_orientation_valid(element_type, orientation) &&
           (input_inverters.empty() || input_inverters.size() == input_count) &&
           (output_inverters.empty() || output_inverters.size() == output_count) &&
           // clock generator
           attrs_clock_generator.has_value() == (element_type == clock_generator) &&
           (!attrs_clock_generator || attrs_clock_generator->is_valid());
}

auto LogicItemDefinition::format() const -> std::string {
    return fmt::format("({}, input_count = {}, output_count = {}, {})", element_type,
                       input_count, output_count, orientation);
}

}  // namespace logicsim