
#include "layout_calculation_type.h"

#include "circuit.h"

namespace logicsim {

auto to_layout_calculation_data(const Schematic& schematic, const Layout& layout,
                                element_id_t element_id) -> layout_calculation_data_t {
    const auto element = schematic.element(element_id);

    return layout_calculation_data_t {
        .segment_tree = layout.segment_tree(element_id),
        .input_count = element.input_count(),
        .output_count = element.output_count(),
        .internal_state_count = 0,  // TODO get count fromm schematic when implemented
        .position = layout.position(element_id),
        .orientation = layout.orientation(element_id),
        .element_type = element.element_type(),
    };
}

auto to_layout_calculation_data(const Circuit& circuit, element_id_t element_id)
    -> layout_calculation_data_t {
    return to_layout_calculation_data(circuit.schematic(), circuit.layout(), element_id);
}

}  // namespace logicsim