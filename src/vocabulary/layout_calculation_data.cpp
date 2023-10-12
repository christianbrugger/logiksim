#include "vocabulary/layout_calculation_data.h"

#include "vocabulary/element_definition.h"
#include "vocabulary/placed_element.h"

#include <fmt/core.h>

namespace logicsim {

auto layout_calculation_data_t::format() const -> std::string {
    return fmt::format(
        "layout_calculation_data_t("
        "element_type={}, position={}, input_count={}, "
        "output_count={}, orientation={}, internal_state_count={}"
        ")",
        element_type, position, input_count, output_count, orientation,
        internal_state_count);
}

//
// Conversion
//

auto to_layout_calculation_data(const ElementDefinition& definition, point_t position)
    -> layout_calculation_data_t {
    return layout_calculation_data_t {
        .internal_state_count = std::size_t {0},  // TODO ???
        .position = position,
        .input_count = definition.input_count,
        .output_count = definition.input_count,
        .orientation = definition.orientation,
        .element_type = definition.element_type,
    };
}

auto to_layout_calculation_data(const PlacedElement& element)
    -> layout_calculation_data_t {
    return to_layout_calculation_data(element.definition, element.position);
}

}  // namespace logicsim
