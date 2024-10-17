#include "core/vocabulary/layout_calculation_data.h"

#include "core/vocabulary/logicitem_definition.h"
#include "core/vocabulary/placed_logicitem.h"

#include <fmt/core.h>

namespace logicsim {

auto layout_calculation_data_t::format() const -> std::string {
    return fmt::format(
        "layout_calculation_data_t("
        "type={}, position={}, input_count={}, "
        "output_count={}, orientation={}, internal_state_count={}"
        ")",
        logicitem_type, position, input_count, output_count, orientation,
        internal_state_count);
}

//
// Conversion
//

auto to_layout_calculation_data(const LogicItemDefinition& definition,
                                point_t position) -> layout_calculation_data_t {
    return layout_calculation_data_t {
        .internal_state_count = std::size_t {0},  // TODO ???
        .position = position,
        .input_count = definition.input_count,
        .output_count = definition.input_count,
        .orientation = definition.orientation,
        .logicitem_type = definition.logicitem_type,
    };
}

auto to_layout_calculation_data(const PlacedLogicItem& element)
    -> layout_calculation_data_t {
    return to_layout_calculation_data(element.definition, element.position);
}

}  // namespace logicsim
