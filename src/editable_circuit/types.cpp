#include "editable_circuit/types.h"

#include "exceptions.h"
#include "layout_calculations.h"

#include <fmt/core.h>

namespace logicsim {

template <>
auto format(LineSegmentType type) -> std::string {
    switch (type) {
        using enum LineSegmentType;

        case horizontal_first:
            return "horizontal_first";
        case vertical_first:
            return "vertical_first";
    }
    throw_exception("unknown LineSegmentType");
}

auto LogicItemDefinition::is_valid() const -> bool {
    return is_input_output_count_valid(element_type, input_count, output_count)
           && is_orientation_valid(element_type, orientation);
}

auto LogicItemDefinition::format() const -> std::string {
    return fmt::format("({}, input_count = {}, output_count = {}, {})", element_type,
                       input_count, output_count, orientation);
}

}  // namespace logicsim