#ifndef LOGICSIM_GEOMETRY_LAYOUT_H
#define LOGICSIM_GEOMETRY_LAYOUT_H

namespace logicsim {

struct point_t;
struct ElementDefinition;
struct PlacedElement;
struct layout_calculation_data_t;

[[nodiscard]] auto to_layout_calculation_data(const ElementDefinition& definition,
                                              point_t position)
    -> layout_calculation_data_t;
[[nodiscard]] auto to_layout_calculation_data(const PlacedElement& element)
    -> layout_calculation_data_t;

}  // namespace logicsim

#endif
