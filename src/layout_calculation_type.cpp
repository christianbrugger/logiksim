
#include "layout_calculation_type.h"

#include "layout.h"

namespace logicsim {

auto to_layout_calculation_data(const Layout& layout, element_id_t element_id)
    -> layout_calculation_data_t {
    const auto element = layout.element(element_id);

    return layout_calculation_data_t {
        .input_count = element.input_count(),
        .output_count = element.output_count(),
        .internal_state_count = 0,  // TODO get count fromm schematic when implemented
        .position = element.position(),
        .orientation = element.orientation(),
        .element_type = element.element_type(),
    };
}
}  // namespace logicsim